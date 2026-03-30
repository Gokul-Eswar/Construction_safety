const express = require('express');
const router = express.Router();
const db = require('../db');
const configManager = require('./configManager');
const mqttService = require('./mqttService');

/**
 * Filter sensitive information from config before sending to frontend.
 */
const filterConfig = (config) => {
    if (!config) return {};
    const filtered = JSON.parse(JSON.stringify(config));
    
    if (filtered.streams) {
        filtered.streams = filtered.streams.map(stream => {
            if (stream.uri) {
                // Strip credentials from RTSP/HTTP URIs
                // Format: rtsp://user:pass@host:port/path -> rtsp://***:***@host:port/path
                stream.uri = stream.uri.replace(/(rtsp:\/\/|http:\/\/)([^:]+):([^@]+)@/, '$1***:***@');
            }
            return stream;
        });
    }

    if (filtered.mqtt && filtered.mqtt.password) {
        filtered.mqtt.password = '***';
    }

    return filtered;
};

// Helper to get zone name
const getZoneName = (id) => {
    const config = configManager.getConfig();
    const normalizedId = String(id);
    // Check in streams
    if (config.streams) {
        for (const stream of config.streams) {
            if (stream.zones) {
                const zone = stream.zones.find(z => String(z.id) === normalizedId);
                if (zone) return zone.name;
            }
        }
    }
    // Fallback for legacy format or missing zones
    if (config.zones) {
        const zone = config.zones.find(z => String(z.id) === normalizedId);
        return zone ? zone.name : `Zone ${id}`;
    }
    return `Zone ${id}`;
};

// GET /api/violations
router.get('/violations', (req, res) => {
    const parsedLimit = Number.parseInt(req.query.limit, 10);
    const parsedOffset = Number.parseInt(req.query.offset, 10);
    const limit = Number.isFinite(parsedLimit) ? Math.min(Math.max(parsedLimit, 1), 200) : 10;
    const offset = Number.isFinite(parsedOffset) ? Math.max(parsedOffset, 0) : 0;

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

// GET /api/stats
router.get('/stats', (req, res) => {
    const today = new Date().toISOString().split('T')[0];
    
    const sql = `SELECT count(*) as count FROM violations WHERE timestamp LIKE ?`;
    db.get(sql, [`${today}%`], (err, row) => {
        if (err) return res.status(500).json({ error: err.message });
        
        const config = configManager.getConfig();
        const activeStreams = config.streams ? config.streams.length : 0;

        res.json({
            today_violations: row.count,
            system_status: 'online',
            active_streams: activeStreams
        });
    });
});

// GET /api/config
router.get('/config', (req, res) => {
    const config = configManager.getConfig();
    res.json(filterConfig(config));
});

// POST /api/config (Legacy/Full)
router.post('/config', (req, res) => {
    const newConfig = req.body;
    if (!newConfig.streams && !newConfig.zones) {
        return res.status(400).json({ error: "Invalid config format" });
    }
    
    if (configManager.saveConfig(newConfig)) {
        res.json({ success: true });
    } else {
        res.status(500).json({ error: "Failed to save config" });
    }
});

// POST /api/config/global
router.post('/config/global', (req, res) => {
    if (!req.body || typeof req.body !== 'object') {
        return res.status(400).json({ error: 'Invalid payload' });
    }

    if (Object.prototype.hasOwnProperty.call(req.body, 'alert_cooldown')) {
        const cooldown = Number.parseInt(req.body.alert_cooldown, 10);
        if (!Number.isFinite(cooldown) || cooldown < 0) {
            return res.status(400).json({ error: 'alert_cooldown must be a non-negative integer' });
        }
    }

    if (configManager.updateGlobalSettings(req.body)) {
        res.json({ success: true, config: filterConfig(configManager.getConfig()) });
    } else {
        res.status(500).json({ error: "Failed to update global settings" });
    }
});

// POST /api/config/streams
router.post('/config/streams', (req, res) => {
    if (!Array.isArray(req.body)) {
        return res.status(400).json({ error: 'Payload must be an array of streams' });
    }

    if (configManager.updateStreams(req.body)) {
        res.json({ success: true, config: filterConfig(configManager.getConfig()) });
    } else {
        res.status(500).json({ error: "Failed to update streams" });
    }
});

// POST /api/system/restart
router.post('/system/restart', async (req, res) => {
    try {
        await mqttService.sendRestartCommand();
        res.json({ success: true, message: "Restart signal sent. Engine should reboot shortly." });
    } catch (err) {
        res.status(503).json({ error: "Failed to send restart signal: " + err.message });
    }
});

module.exports = router;

