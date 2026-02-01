const express = require('express');
const cors = require('cors');
const db = require('./db');
const fs = require('fs');
const path = require('path');
const { exec } = require('child_process');

const app = express();
const PORT = 3001;

app.use(cors());
app.use(express.json());

// Load project config to get Zone Names
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

// API: System Restart (Mock)
app.post('/api/system/restart', (req, res) => {
    console.log("Restart request received. In a real deployment, this would trigger a service restart.");
    // For now, we just return success. 
    // In a supervised environment (e.g. Systemd or PM2), we could execute a restart command.
    res.json({ success: true, message: "Restart signal sent to system controller." });
});

// Serve Frontend in Production
if (process.env.NODE_ENV === 'production') {
    app.use(express.static(path.join(__dirname, '../frontend/dist')));
    
    app.get('*', (req, res) => {
        res.sendFile(path.join(__dirname, '../frontend/dist/index.html'));
    });
}

app.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
});