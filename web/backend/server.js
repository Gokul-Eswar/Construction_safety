const express = require('express');
const cors = require('cors');
const db = require('./db');
const fs = require('fs');
const path = require('path');
const { exec } = require('child_process');
const http = require('http');
const { Server } = require('socket.io');
const mqtt = require('mqtt');

const app = express();
const PORT = 3001;

app.use(cors());
app.use(express.json());

// Basic Auth Middleware
const authMiddleware = (req, res, next) => {
    loadConfig(); // Ensure fresh config
    if (!projectConfig.auth || !projectConfig.auth.username) return next();

    const b64auth = (req.headers.authorization || '').split(' ')[1] || '';
    const [login, password] = Buffer.from(b64auth, 'base64').toString().split(':');

    if (login && password && login === projectConfig.auth.username && password === projectConfig.auth.password) {
        return next();
    }

    res.set('WWW-Authenticate', 'Basic realm="Sentinel Safety Dashboard"');
    res.status(401).send('Authentication required.');
};

// Protect API
app.use('/api', authMiddleware);

// Setup Socket.IO
const server = http.createServer(app);
const io = new Server(server, {
    cors: {
        origin: "*", // Allow all for prototype
        methods: ["GET", "POST"]
    }
});

// Load project config
const configPath = path.resolve(__dirname, '../../config.json');
let projectConfig = {};

const loadConfig = () => {
    try {
        const rawData = fs.readFileSync(configPath);
        projectConfig = JSON.parse(rawData);
    } catch (error) {
        console.error("Could not load config.json:", error);
    }
};
loadConfig();

// Setup MQTT Client
let mqttClient = null;
const setupMQTT = () => {
    if (mqttClient) mqttClient.end();
    
    // Determine MQTT host (ENV override or config)
    const host = process.env.MQTT_HOST || (projectConfig.mqtt ? projectConfig.mqtt.host : 'localhost');
    const port = projectConfig.mqtt ? projectConfig.mqtt.port : 1883;
    const topic = projectConfig.mqtt ? projectConfig.mqtt.topic : 'safety/alerts';

    console.log(`Connecting to MQTT at mqtt://${host}:${port}`);
    mqttClient = mqtt.connect(`mqtt://${host}:${port}`);

    mqttClient.on('connect', () => {
        console.log('MQTT Connected');
        mqttClient.subscribe(topic, (err) => {
            if (!err) console.log(`Subscribed to ${topic}`);
        });
    });

    mqttClient.on('message', (topic, message) => {
        try {
            const payload = JSON.parse(message.toString());
            // Broadcast to all connected websocket clients
            io.emit('violation_alert', payload);
        } catch (e) {
            console.error('Failed to parse MQTT message');
        }
    });
};

setupMQTT(); // Initial setup

// Socket.IO Connection
io.on('connection', (socket) => {
    console.log('New client connected');
    socket.on('disconnect', () => console.log('Client disconnected'));
});

// Helper to get zone name
const getZoneName = (id) => {
    // Check in streams
    if (projectConfig.streams) {
        for (const stream of projectConfig.streams) {
            if (stream.zones) {
                const zone = stream.zones.find(z => z.id === id);
                if (zone) return zone.name;
            }
        }
    }
    // Fallback for legacy format or missing zones
    if (projectConfig.zones) {
        const zone = projectConfig.zones.find(z => z.id === id);
        return zone ? zone.name : `Zone ${id}`;
    }
    return `Zone ${id}`;
};

// API: Get Violations (Paginated)
app.get('/api/violations', (req, res) => {
    const limit = parseInt(req.query.limit) || 10;
    const offset = parseInt(req.query.offset) || 0;

    const sql = `SELECT * FROM violations ORDER BY id DESC LIMIT ? OFFSET ?`;
    const countSql = `SELECT count(*) as count FROM violations`;

    db.get(countSql, [], (err, row) => {
        if (err) return res.status(500).json({ error: err.message });
        const total = row.count;

        db.all(sql, [limit, offset], (err, rows) => {
            if (err) return res.status(500).json({ error: err.message });

            // Enrich with zone names
            const enrichedRows = rows.map(r => ({
                ...r,
                zone_name: getZoneName(r.zone_id)
            }));

            res.json({
                data: enrichedRows,
                pagination: {
                    total,
                    limit,
                    offset
                }
            });
        });
    });
});

// API: Get Stats (Today's counts)
app.get('/api/stats', (req, res) => {
    const today = new Date().toISOString().split('T')[0];
    
    const sql = `SELECT count(*) as count FROM violations WHERE timestamp LIKE ?`;
    db.get(sql, [`${today}%`], (err, row) => {
        if (err) return res.status(500).json({ error: err.message });
        
        // Count active streams
        const activeStreams = projectConfig.streams ? projectConfig.streams.length : 0;

        res.json({
            today_violations: row.count,
            system_status: 'online',
            active_streams: activeStreams
        });
    });
});

// API: Get Config
app.get('/api/config', (req, res) => {
    loadConfig(); // Reload to ensure freshness
    res.json(projectConfig);
});

// API: Update Config (Legacy/Full)
app.post('/api/config', (req, res) => {
    try {
        const newConfig = req.body;
        // Basic validation
        if (!newConfig.streams && !newConfig.zones) {
            return res.status(400).json({ error: "Invalid config format" });
        }
        
        fs.writeFileSync(configPath, JSON.stringify(newConfig, null, 4));
        projectConfig = newConfig;
        
        res.json({ success: true });
    } catch (err) {
        res.status(500).json({ error: "Failed to save config" });
    }
});

// API: Update Global Settings
app.post('/api/config/global', (req, res) => {
    try {
        loadConfig();
        const settings = req.body; // Expects { mqtt: {...}, alert_cooldown: ..., model_path: ... }
        
        if (settings.mqtt) projectConfig.mqtt = settings.mqtt;
        if (settings.alert_cooldown) projectConfig.alert_cooldown = parseInt(settings.alert_cooldown);
        if (settings.model_path) projectConfig.model_path = settings.model_path;
        
        fs.writeFileSync(configPath, JSON.stringify(projectConfig, null, 4));
        res.json({ success: true, config: projectConfig });
    } catch (err) {
        res.status(500).json({ error: "Failed to update global settings" });
    }
});

// API: Update Streams
app.post('/api/config/streams', (req, res) => {
    try {
        loadConfig();
        const streams = req.body; // Expects array of streams
        
        if (!Array.isArray(streams)) return res.status(400).json({ error: "Streams must be an array" });
        
        projectConfig.streams = streams;
        fs.writeFileSync(configPath, JSON.stringify(projectConfig, null, 4));
        res.json({ success: true, config: projectConfig });
    } catch (err) {
        res.status(500).json({ error: "Failed to update streams" });
    }
});

// API: System Restart (Real)
app.post('/api/system/restart', (req, res) => {
    console.log("Restart request received. Sending signal to Engine via MQTT...");
    
    if (mqttClient && mqttClient.connected) {
        mqttClient.publish('safety/control', JSON.stringify({ command: 'restart', initiator: 'web_ui' }), (err) => {
            if (err) {
                console.error("Failed to publish restart command:", err);
                return res.status(500).json({ error: "Failed to send restart signal" });
            }
            
            // Also reload local config
            setTimeout(() => {
                loadConfig();
                setupMQTT();
            }, 2000);
            
            res.json({ success: true, message: "Restart signal sent. Engine should reboot shortly." });
        });
    } else {
        res.status(503).json({ error: "MQTT not connected. Cannot send restart signal." });
    }
});

// Serve Frontend in Production
if (process.env.NODE_ENV === 'production') {
    app.use(express.static(path.join(__dirname, '../frontend/dist')));
    
    app.get('*', (req, res) => {
        res.sendFile(path.join(__dirname, '../frontend/dist/index.html'));
    });
}

server.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
});