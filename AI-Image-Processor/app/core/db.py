import sqlite3
import aiosqlite
from datetime import datetime
from pathlib import Path
from typing import Optional

from app.config import settings

SCHEMA = """
CREATE TABLE IF NOT EXISTS jobs (
    job_id TEXT PRIMARY KEY,    -- unique fingerprint for this event
    device_id TEXT NOT NULL,   -- which camera sent the photo
    created_at TEXT NOT NULL,  -- when it hit the server
    status TEXT NOT NULL DEFAULT 'queued', -- queued, processing, done, or failed
    image_path TEXT,           -- where the jpg is hiding
    result_json_path TEXT,     -- raw ai data
    result_txt_path TEXT,      -- clean text for the sms
    error TEXT,                -- what went wrong (if anything)
    started_at TEXT,           -- when the worker grabbed it
    finished_at TEXT           -- when we finished the whole thing
);
"""

def init_db() -> None:
    "create the table if it doesn't exist yet"
    settings.database_path.parent.mkdir(parents=True, exist_ok=True)

    conn = sqlite3.connect(settings.database_path)
    conn.executescript(SCHEMA)
    conn.commit()
    conn.close()


async def create_job(job_id: str, device_id: str, image_path: str) -> None:
    """toss a new job into the database"""
    async with aiosqlite.connect(settings.database_path) as db:
        await db.execute(
            """
            INSERT INTO jobs (job_id, device_id, created_at, status, image_path)
            VALUES (?, ?, ?, 'queued', ?)
            """,
            (job_id, device_id, datetime.utcnow().isoformat(), image_path),
        )
        await db.commit()

async def get_job(job_id: str) -> Optional[dict]:
    """find a job by its id"""
    async with aiosqlite.connect(settings.database_path) as db:
        db.row_factory = aiosqlite.Row
        async with db.execute( 
            "SELECT * FROM jobs WHERE job_id = ?", (job_id,)
        ) as cursor:
            row = await cursor.fetchone()
            return dict(row) if row else None


def claim_next_job() -> Optional[dict]:
    """grab the oldest job that's still waiting"""
    conn = sqlite3.connect(settings.database_path)
    conn.row_factory = sqlite3.Row
    cursor = conn.cursor()

    # Step 1: find the next one in line
    cursor.execute(
        "SELECT job_id FROM jobs WHERE status = 'queued' ORDER BY created_at ASC LIMIT 1"
    )
    row = cursor.fetchone()
    if not row:
        conn.close()
        return None

    job_id = row["job_id"]
    now = datetime.utcnow().isoformat()

    # Step 2: mark it as ours so nobody else touches it
    cursor.execute(
        "UPDATE jobs SET status = 'processing', started_at = ? WHERE job_id = ?",
        (now, job_id)
    )
    conn.commit()

    # Step 3: get all the info for the worker
    cursor.execute("SELECT * FROM jobs WHERE job_id = ?", (job_id,))
    job = cursor.fetchone()
    conn.close()
    return dict(job) if job else None


def complete_job(job_id: str, result_json_path: str, result_txt_path: str) -> None:
    """mark the job as finished and save the results"""
    conn = sqlite3.connect(settings.database_path)
    conn.execute(
        """
        UPDATE jobs 
        SET status = 'done', 
            result_json_path = ?, 
            result_txt_path = ?, 
            finished_at = ?
        WHERE job_id = ?
        """,
        (result_json_path, result_txt_path, datetime.utcnow().isoformat(), job_id),
    )
    conn.commit()
    conn.close()

def fail_job(job_id: str, error: str) -> None:
    """something broke cuh. log the error"""
    conn = sqlite3.connect(settings.database_path)
    conn.execute(
        """
        UPDATE jobs 
        SET status = 'failed', 
            error = ?, 
            finished_at = ?
        WHERE job_id = ?
        """,
        (error, datetime.utcnow().isoformat(), job_id),
    )
    conn.commit()
    conn.close()
