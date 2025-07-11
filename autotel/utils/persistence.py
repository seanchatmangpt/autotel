import sqlite3
import json
from typing import Optional, List, Dict
from pathlib import Path

DB_PATH = Path("autotel_process_instances.sqlite3")

CREATE_TABLE_SQL = """
CREATE TABLE IF NOT EXISTS process_instances (
    instance_id TEXT PRIMARY KEY,
    process_definition_id TEXT,
    status TEXT,
    variables TEXT,
    start_time TEXT,
    end_time TEXT
);
"""

def get_connection():
    conn = sqlite3.connect(DB_PATH)
    conn.execute(CREATE_TABLE_SQL)
    return conn

def save_instance(instance_id: str, process_definition_id: str, status: str, variables: dict, start_time: str, end_time: Optional[str]):
    conn = get_connection()
    with conn:
        conn.execute(
            """
            INSERT OR REPLACE INTO process_instances (instance_id, process_definition_id, status, variables, start_time, end_time)
            VALUES (?, ?, ?, ?, ?, ?)
            """,
            (instance_id, process_definition_id, status, json.dumps(variables), start_time, end_time)
        )
    conn.close()

def load_instance(instance_id: str) -> Optional[Dict]:
    conn = get_connection()
    cur = conn.cursor()
    cur.execute("SELECT * FROM process_instances WHERE instance_id = ?", (instance_id,))
    row = cur.fetchone()
    conn.close()
    if row:
        return {
            "instance_id": row[0],
            "process_definition_id": row[1],
            "status": row[2],
            "variables": json.loads(row[3]) if row[3] else {},
            "start_time": row[4],
            "end_time": row[5],
        }
    return None

def list_instances() -> List[Dict]:
    conn = get_connection()
    cur = conn.cursor()
    cur.execute("SELECT * FROM process_instances")
    rows = cur.fetchall()
    conn.close()
    return [
        {
            "instance_id": row[0],
            "process_definition_id": row[1],
            "status": row[2],
            "variables": json.loads(row[3]) if row[3] else {},
            "start_time": row[4],
            "end_time": row[5],
        }
        for row in rows
    ]

def delete_instance(instance_id: str):
    conn = get_connection()
    with conn:
        conn.execute("DELETE FROM process_instances WHERE instance_id = ?", (instance_id,))
    conn.close() 