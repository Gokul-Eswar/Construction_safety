/**
 * Mock Inference Engine Server
 * Provides health checks and mock inference responses
 * Publishes heartbeats to MQTT to signal system is online
 * Allows web dashboard to operate without C++ engine
 */

const http = require('http');
const mqtt = require('mqtt');

const PORT = 8081;
const MQTT_HOST = process.env.MQTT_HOST || 'localhost';
const MQTT_PORT = process.env.MQTT_PORT || 1883;

// Connect to MQTT and publish heartbeats
let mqttClient = null;

function connectMQTT() {
    mqttClient = mqtt.connect(`mqtt://${MQTT_HOST}:${MQTT_PORT}`);

    mqttClient.on('connect', () => {
        console.log(`✓ Connected to MQTT at ${MQTT_HOST}:${MQTT_PORT}`);
        
        // Publish heartbeat every 2 seconds
        setInterval(() => {
            const heartbeat = {
                status: 'healthy',
                engine: 'mock_inference_engine',
                timestamp: new Date().toISOString(),
                version: '1.0.0'
            };
            mqttClient.publish('safety/heartbeat', JSON.stringify(heartbeat), (err) => {
                if (!err) {
                    console.log(`[HEARTBEAT] Published at ${heartbeat.timestamp}`);
                }
            });
        }, 2000);
    });

    mqttClient.on('error', (err) => {
        console.error(`✗ MQTT Error: ${err.message}`);
        // Retry connection after 5 seconds
        setTimeout(connectMQTT, 5000);
    });
}

const server = http.createServer((req, res) => {
    // CORS headers
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
    res.setHeader('Content-Type', 'application/json');

    if (req.method === 'OPTIONS') {
        res.writeHead(200);
        res.end();
        return;
    }

    // Health check endpoint
    if (req.url === '/health' || req.url === '/status') {
        res.writeHead(200);
        res.end(JSON.stringify({
            status: 'healthy',
            engine: 'mock_inference_engine',
            version: '1.0.0',
            timestamp: new Date().toISOString()
        }));
        return;
    }

    // Inference endpoint (mock)
    if (req.url === '/infer' && req.method === 'POST') {
        res.writeHead(200);
        res.end(JSON.stringify({
            success: true,
            detections: []
        }));
        return;
    }

    // Root endpoint
    if (req.url === '/' || req.url === '') {
        res.writeHead(200);
        res.end(JSON.stringify({
            service: 'Construction Safety - Inference Engine (Mock)',
            mode: 'mock',
            ready: true
        }));
        return;
    }

    res.writeHead(404);
    res.end(JSON.stringify({ error: 'Not found' }));
});

server.listen(PORT, () => {
    console.log(`✓ Mock Inference Engine running on http://localhost:${PORT}`);
    console.log(`✓ Connecting to MQTT broker...`);
    connectMQTT();
    console.log(`✓ Dashboard will show SYSTEM ONLINE once MQTT heartbeat is received`);
});
