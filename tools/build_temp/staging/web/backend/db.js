const sqlite3 = require('sqlite3').verbose();
const path = require('path');

// Path to the database file in the project root
const dbPath = path.resolve(__dirname, '../../safety_violations.db');

const db = new sqlite3.Database(dbPath, sqlite3.OPEN_READONLY, (err) => {
    if (err) {
        console.error('Error connecting to database:', err.message);
        console.error('Expected DB path:', dbPath);
    } else {
        console.log('Connected to SQLite database at:', dbPath);
    }
});

module.exports = db;
