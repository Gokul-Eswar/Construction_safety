const express = require('express');
const cors = require('cors');
const db = require('./db');
const fs = require('fs');
const path = require('path');

const app = express();
const PORT = 3001;

app.use(cors());
app.use(express.json());

// Load project config to get Zone Names
const configPath = path.resolve(__dirname, '../../config.json');
let projectConfig = {};
try {
    const rawData = fs.readFileSync(configPath);
    projectConfig = JSON.parse(rawData);
} catch (error) {
    console.error("Could not load config.json:", error);
}

// Helper to get zone name
const getZoneName = (id) => {
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
    // Basic stat: Count violations today
    // SQLite 'now' is in UTC, assume local or just grab all for prototype if needed.
    // Let's grab total active today.
    const today = new Date().toISOString().split('T')[0];
    
    const sql = `SELECT count(*) as count FROM violations WHERE timestamp LIKE ?`;
    db.get(sql, [`${today}%`], (err, row) => {
        if (err) return res.status(500).json({ error: err.message });
        res.json({
            today_violations: row.count,
            system_status: 'online' // Mock status
        });
    });
});

app.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
});
