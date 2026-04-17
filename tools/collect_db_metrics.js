const path = require('path');
const sqlite3 = require('../web/backend/node_modules/sqlite3').verbose();

const dbPath = process.argv[2]
  ? path.resolve(process.argv[2])
  : path.resolve(__dirname, '..', 'safety_violations.db');

const db = new sqlite3.Database(dbPath, sqlite3.OPEN_READONLY, (err) => {
  if (err) {
    console.error('[DB] Failed to open database:', err.message);
    process.exit(1);
  }
});

function all(sql, params = []) {
  return new Promise((resolve, reject) => {
    db.all(sql, params, (err, rows) => {
      if (err) reject(err);
      else resolve(rows);
    });
  });
}

(async () => {
  try {
    const tables = await all("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name");
    const hasViolations = tables.some((t) => t.name === 'violations');

    const output = {
      generated_at: new Date().toISOString(),
      database_path: dbPath,
      tables: tables.map((t) => t.name),
      violations: null,
      notes: []
    };

    if (!hasViolations) {
      output.notes.push('Table violations not found.');
      console.log(JSON.stringify(output, null, 2));
      db.close();
      return;
    }

    const summary = await all(
      `SELECT
        COUNT(*) AS total,
        MIN(timestamp) AS first_ts,
        MAX(timestamp) AS last_ts,
        ROUND(AVG(confidence), 4) AS avg_conf,
        ROUND(MIN(confidence), 4) AS min_conf,
        ROUND(MAX(confidence), 4) AS max_conf,
        SUM(CASE WHEN uploaded=1 THEN 1 ELSE 0 END) AS uploaded_count,
        SUM(CASE WHEN uploaded=0 THEN 1 ELSE 0 END) AS pending_count
      FROM violations`
    );

    const daily = await all(
      `SELECT
        substr(timestamp, 1, 10) AS day,
        COUNT(*) AS count
      FROM violations
      GROUP BY day
      ORDER BY day DESC
      LIMIT 30`
    );

    const zone = await all(
      `SELECT
        zone_id,
        COUNT(*) AS count,
        ROUND(AVG(confidence), 4) AS avg_conf
      FROM violations
      GROUP BY zone_id
      ORDER BY count DESC, zone_id ASC`
    );

    const camera = await all(
      `SELECT
        COALESCE(NULLIF(camera_id, ''), 'unknown') AS camera_id,
        COUNT(*) AS count
      FROM violations
      GROUP BY COALESCE(NULLIF(camera_id, ''), 'unknown')
      ORDER BY count DESC, camera_id ASC`
    );

    const latest = await all(
      `SELECT id, timestamp, zone_id, confidence, object_id, uploaded, camera_id
      FROM violations
      ORDER BY id DESC
      LIMIT 10`
    );

    output.violations = {
      summary: summary[0] || {},
      by_day_last_30: daily,
      by_zone: zone,
      by_camera: camera,
      latest_10: latest
    };

    console.log(JSON.stringify(output, null, 2));
  } catch (err) {
    console.error('[DB] Query failed:', err.message);
    process.exitCode = 1;
  } finally {
    db.close();
  }
})();
