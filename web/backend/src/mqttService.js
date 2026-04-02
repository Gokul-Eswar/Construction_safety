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
        
        // Subscribe to Alerts
        mqttClient.subscribe(topic, (err) => {
            if (!err) console.log(`[MQTT] Subscribed to ${topic}`);
        });
        
        // Subscribe to Heartbeat
        mqttClient.subscribe('safety/heartbeat', (err) => {
            if (!err) console.log(`[MQTT] Subscribed to safety/heartbeat`);
        });

        // Subscribe to Telemetry
        mqttClient.subscribe('safety/telemetry', (err) => {
            if (!err) console.log(`[MQTT] Subscribed to safety/telemetry`);
        });

        // Subscribe to Cloud Sync
        mqttClient.subscribe('safety/cloud_sync', (err) => {
            if (!err) console.log(`[MQTT] Subscribed to safety/cloud_sync`);
        });
    });

    mqttClient.on('message', (topic, message) => {
        if (!ioInstance) return;
        
        try {
            const payload = JSON.parse(message.toString());
            
            if (topic === 'safety/heartbeat') {
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
                console.log('[MQTT→Socket.IO] Broadcasting violation_alert to all clients');
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
        await publish('safety/control', JSON.stringify({ command: 'restart', initiator: 'web_ui' }));
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
