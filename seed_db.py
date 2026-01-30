import sqlite3
import datetime

db_path = "safety_violations.db"
conn = sqlite3.connect(db_path)
c = conn.cursor()
c.execute('''CREATE TABLE IF NOT EXISTS violations (
             id INTEGER PRIMARY KEY AUTOINCREMENT,
             timestamp TEXT NOT NULL,
             zone_id INTEGER NOT NULL,
             confidence REAL NOT NULL,
             object_id INTEGER,
             is_active INTEGER DEFAULT 1
             )''')

# Insert some dummy data
timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
c.execute("INSERT INTO violations (timestamp, zone_id, confidence, object_id) VALUES (?, ?, ?, ?)", 
          (timestamp, 1, 0.95, -1))

conn.commit()
conn.close()
print("Database created and seeded.")
