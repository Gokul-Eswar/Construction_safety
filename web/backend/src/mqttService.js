const mqtt = require('mqtt');
const configManager = require('./configManager');

let mqttClient = null;
let ioInstance = null;

const setup = (io) => {
    ioInstance = io;
    connect();
};

const connect = () => {
    if (mqttClient) mqttClient.end();

    const config = configManager.getConfig();
    // Determine MQTT host (ENV override or config)
    const host = process.env.MQTT_HOST || (config.mqtt ? config.mqtt.host : 'localhost');
    const port = config.mqtt ? config.mqtt.port : 1883;
    const topic = config.mqtt ? config.mqtt.topic : 'safety/alerts';

    console.log(`[MQTT] Connecting to mqtt://${host}:${port}`);
    mqttClient = mqtt.connect(`mqtt://${host}:${port}`);

    mqttClient.on('connect', () => {
        console.log('[MQTT] Connected');
        
        // Subscribe to versioned topics (v1)
        const v1_topics = [
            'safety/v1/violations',
            'safety/v1/heartbeat',
            'safety/v1/telemetry',
            'safety/v1/cloud_sync',
            'safety/v1/handshake'
        ];
        
        // Also subscribe to unversioned topics for backward compatibility
        const legacy_topics = [
            topic,  // Config-specified violations topic
            'safety/alerts',
            'safety/heartbeat',
            'safety/telemetry',
            'safety/cloud_sync'
        ];
        
        const allTopics = [...new Set([...v1_topics, ...legacy_topics])];
        
        allTopics.forEach(t => {
            mqttClient.subscribe(t, (err) => {
                if (!err) console.log(`[MQTT] Subscribed to ${t}`);
                else console.error(`[MQTT] Failed to subscribe to ${t}: ${err.message}`);
            });
        });
    });

    mqttClient.on('message', (topic, message) => {
        if (!ioInstance) return;
        
        try {
            const payload = JSON.parse(message.toString());
            
            // Route based on versioned topics (preferred) or legacy topics
            if (topic === 'safety/v1/violations' || topic === 'safety/alerts' || topic.includes('safety/v1/')) {
                // Violation alert - check v1 first, then legacy
                if (topic.includes('violation') || topic === 'safety/alerts' || topic === 'safety/v1/violations') {
                    console.log('[MQTT→Socket.IO] Broadcasting violation_alert from topic:', topic);
                    ioInstance.emit('violation_alert', payload);
                } else if (topic === 'safety/v1/heartbeat' || topic === 'safety/heartbeat') {
                    console.log('[MQTT→Socket.IO] Broadcasting system_heartbeat from topic:', topic);
                    ioInstance.emit('system_heartbeat', payload);
                } else if (topic === 'safety/v1/telemetry' || topic === 'safety/telemetry') {
                    console.log('[MQTT→Socket.IO] Broadcasting system_telemetry from topic:', topic);
                    ioInstance.emit('system_telemetry', payload);
                } else if (topic === 'safety/v1/cloud_sync' || topic === 'safety/cloud_sync') {
                    console.log('[MQTT→Socket.IO] Broadcasting cloud_sync_event from topic:', topic);
                    ioInstance.emit('cloud_sync_event', payload);
                } else if (topic === 'safety/v1/handshake') {
                    console.log('[MQTT] Received handshake from engine:', JSON.stringify(payload));
                } else {
                    console.log('[MQTT→Socket.IO] Broadcasting violation_alert from topic:', topic);
                    ioInstance.emit('violation_alert', payload);
                }
            } else if (topic === 'safety/heartbeat') {
                console.log('[MQTT→Socket.IO] Broadcasting system_heartbeat to all clients');
                ioInstance.emit('system_heartbeat', payload);
            } else if (topic === 'safety/telemetry') {
                console.log('[MQTT→Socket.IO] Broadcasting system_telemetry to all clients');
                ioInstance.emit('system_telemetry', payload);
            } else if (topic === 'safety/cloud_sync') {
                console.log('[MQTT→Socket.IO] Broadcasting cloud_sync_event to all clients');
                ioInstance.emit('cloud_sync_event', payload);
            } else {
                // Default: Violation Alert
                console.log('[MQTT→Socket.IO] Broadcasting violation_alert to all clients from topic:', topic);
                ioInstance.emit('violation_alert', payload);
            }
        } catch (e) {
            console.error('[MQTT] Failed to parse message from', topic, ':', e.message);
        }
    });

    mqttClient.on('error', (err) => {
        console.error('[MQTT] Error:', err.message);
    });
};

const publish = (topic, message) => {
    return new Promise((resolve, reject) => {
        if (mqttClient && mqttClient.connected) {
            mqttClient.publish(topic, message, (err) => {
                if (err) reject(err);
                else resolve();
            });
        } else {
            reject(new Error("MQTT not connected"));
        }
    });
};

const sendRestartCommand = async () => {
    try {
        // Publish control command to versioned topic
        await publish('safety/v1/control', JSON.stringify({ 
            version: "1.0",
            command: 'restart', 
            initiator: 'web_ui',
            reason: 'user_requested',
            timestamp: new Date().toISOString()
        }));
        // Reload local config connection after a short delay
        setTimeout(() => {
            connect();
        }, 2000);
        return true;
    } catch (err) {
        console.error("[MQTT] Failed to send restart command:", err);
        throw err;
    }
};

module.exports = {
    setup,
    connect, // Exported to allow reconnection
    sendRestartCommand
};
