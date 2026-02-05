const sqlite3 = require('sqlite3').verbose();
const path = require('path');

// Use DB_PATH env var if available, otherwise resolve relative to project root
const dbPath = process.env.DB_PATH || path.resolve(process.cwd(), '../../safety_violations.db');

const db = new sqlite3.Database(dbPath, sqlite3.OPEN_READONLY, (err) => {
    if (err) {
        console.error('Error connecting to database:', err.message);
        console.error('Expected DB path:', dbPath);
    } else {
        console.log('Connected to SQLite database at:', dbPath);
    }
});

module.exports = db;
