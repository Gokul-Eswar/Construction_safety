const express = require('express');
const router = express.Router();
const db = require('../db');
const configManager = require('./configManager');
const mqttService = require('./mqttService');

// Middleware to inject dependencies if needed, or just import them directly.

// Helper to get zone name
const getZoneName = (id) => {
    const config = configManager.getConfig();
    // Check in streams
    if (config.streams) {
        for (const stream of config.streams) {
            if (stream.zones) {
                const zone = stream.zones.find(z => z.id === id);
                if (zone) return zone.name;
            }
        }
    }
    // Fallback for legacy format or missing zones
    if (config.zones) {
        const zone = config.zones.find(z => z.id === id);
        return zone ? zone.name : `Zone ${id}`;
    }
    return `Zone ${id}`;
};

// GET /api/violations
router.get('/violations', (req, res) => {
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
    res.json(configManager.getConfig());
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
    if (configManager.updateGlobalSettings(req.body)) {
        res.json({ success: true, config: configManager.getConfig() });
    } else {
        res.status(500).json({ error: "Failed to update global settings" });
    }
});

// POST /api/config/streams
router.post('/config/streams', (req, res) => {
    if (configManager.updateStreams(req.body)) {
        res.json({ success: true, config: configManager.getConfig() });
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
