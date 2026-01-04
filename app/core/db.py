import sqlite3
import aiosqlite
import asyncio
import time
import logging
from datetime import datetime
from pathlib import Path
from typing import Optional

from app.config import settings

logger = logging.getLogger(__name__)

# Busy timeout in milliseconds - wait up to 5 seconds for locks
BUSY_TIMEOUT_MS = 5000
MAX_RETRIES = 3
RETRY_DELAY_S = 0.1

SCHEMA = """
-- Enable WAL mode for better concurrency (one writer + multiple readers)
PRAGMA journal_mode=WAL;
PRAGMA busy_timeout=5000;

CREATE TABLE IF NOT EXISTS jobs (
    job_id TEXT PRIMARY KEY,
    device_id TEXT NOT NULL,
    created_at TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'queued',
    image_path TEXT,
    result_json_path TEXT,
    result_txt_path TEXT,
    error TEXT,
    started_at TEXT,
    finished_at TEXT
);

CREATE TABLE IF NOT EXISTS heartbeats (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id TEXT NOT NULL,
    timestamp TEXT NOT NULL,
    battery_voltage REAL,
    battery_percent INTEGER,
    wifi_rssi INTEGER,
    free_heap INTEGER,
    uptime_ms INTEGER
);

CREATE INDEX IF NOT EXISTS idx_heartbeats_device ON heartbeats(device_id, timestamp DESC);
"""

def get_sync_connection() -> sqlite3.Connection:
    """Get a sync connection with proper settings for concurrency."""
    conn = sqlite3.connect(settings.database_path, timeout=BUSY_TIMEOUT_MS / 1000)
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute(f"PRAGMA busy_timeout={BUSY_TIMEOUT_MS}")
    return conn

def init_db() -> None:
    """Create tables with WAL mode for concurrent access."""
    settings.database_path.parent.mkdir(parents=True, exist_ok=True)

    conn = get_sync_connection()
    conn.executescript(SCHEMA)
    conn.commit()
    conn.close()
    logger.info(f"Database initialized with WAL mode at {settings.database_path}")


async def get_async_connection() -> aiosqlite.Connection:
    """Get an async connection with proper settings for concurrency."""
    db = await aiosqlite.connect(settings.database_path, timeout=BUSY_TIMEOUT_MS / 1000)
    await db.execute("PRAGMA journal_mode=WAL")
    await db.execute(f"PRAGMA busy_timeout={BUSY_TIMEOUT_MS}")
    return db


async def create_job(job_id: str, device_id: str, image_path: str) -> None:
    """Insert a new job into the database with retry logic."""
    for attempt in range(MAX_RETRIES):
        try:
            async with aiosqlite.connect(settings.database_path, timeout=BUSY_TIMEOUT_MS / 1000) as db:
                await db.execute("PRAGMA journal_mode=WAL")
                await db.execute(f"PRAGMA busy_timeout={BUSY_TIMEOUT_MS}")
                await db.execute(
                    """
                    INSERT INTO jobs (job_id, device_id, created_at, status, image_path)
                    VALUES (?, ?, ?, 'queued', ?)
                    """,
                    (job_id, device_id, datetime.utcnow().isoformat(), image_path),
                )
                await db.commit()
                return
        except sqlite3.OperationalError as e:
            if "database is locked" in str(e) and attempt < MAX_RETRIES - 1:
                logger.warning(f"Database locked, retrying ({attempt + 1}/{MAX_RETRIES})...")
                await asyncio.sleep(RETRY_DELAY_S * (attempt + 1))
            else:
                raise

async def get_job(job_id: str) -> Optional[dict]:
    """Fetch a job by ID. Returns None if not found."""
    async with aiosqlite.connect(settings.database_path, timeout=BUSY_TIMEOUT_MS / 1000) as db:
        await db.execute("PRAGMA journal_mode=WAL")
        db.row_factory = aiosqlite.Row
        async with db.execute( 
            "SELECT * FROM jobs WHERE job_id = ?", (job_id,)
        ) as cursor:
            row = await cursor.fetchone()
            return dict(row) if row else None


def claim_next_job() -> Optional[dict]:
    """Atomically claim the oldest queued job with retry logic."""
    for attempt in range(MAX_RETRIES):
        try:
            conn = get_sync_connection()
            conn.row_factory = sqlite3.Row
            cursor = conn.cursor()

            cursor.execute(
                """
                UPDATE jobs
                SET status = 'processing', started_at = ?
                WHERE job_id = (
                    SELECT job_id FROM jobs WHERE status = 'queued' ORDER BY created_at ASC LIMIT 1
                )
                RETURNING *
                """,
                (datetime.utcnow().isoformat(),)
            )
            row = cursor.fetchone()
            conn.commit()
            conn.close()
            return dict(row) if row else None
        except sqlite3.OperationalError as e:
            if "database is locked" in str(e) and attempt < MAX_RETRIES - 1:
                logger.warning(f"Database locked in claim_next_job, retrying ({attempt + 1}/{MAX_RETRIES})...")
                time.sleep(RETRY_DELAY_S * (attempt + 1))
            else:
                raise


def complete_job(job_id: str, result_json_path: str, result_txt_path: str) -> None:
    """Mark a job as successfully completed with retry logic."""
    for attempt in range(MAX_RETRIES):
        try:
            conn = get_sync_connection()
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
            return
        except sqlite3.OperationalError as e:
            if "database is locked" in str(e) and attempt < MAX_RETRIES - 1:
                logger.warning(f"Database locked in complete_job, retrying ({attempt + 1}/{MAX_RETRIES})...")
                time.sleep(RETRY_DELAY_S * (attempt + 1))
            else:
                raise


def fail_job(job_id: str, error: str) -> None:
    """Mark a job as failed with retry logic."""
    for attempt in range(MAX_RETRIES):
        try:
            conn = get_sync_connection()
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
            return
        except sqlite3.OperationalError as e:
            if "database is locked" in str(e) and attempt < MAX_RETRIES - 1:
                logger.warning(f"Database locked in fail_job, retrying ({attempt + 1}/{MAX_RETRIES})...")
                time.sleep(RETRY_DELAY_S * (attempt + 1))
            else:
                raise


async def record_heartbeat(
    device_id: str,
    battery_voltage: Optional[float] = None,
    battery_percent: Optional[int] = None,
    wifi_rssi: Optional[int] = None,
    free_heap: Optional[int] = None,
    uptime_ms: Optional[int] = None,
) -> None:
    """Record a device heartbeat with telemetry data."""
    for attempt in range(MAX_RETRIES):
        try:
            async with aiosqlite.connect(settings.database_path, timeout=BUSY_TIMEOUT_MS / 1000) as db:
                await db.execute("PRAGMA journal_mode=WAL")
                await db.execute(f"PRAGMA busy_timeout={BUSY_TIMEOUT_MS}")
                await db.execute(
                    """
                    INSERT INTO heartbeats (device_id, timestamp, battery_voltage, battery_percent, wifi_rssi, free_heap, uptime_ms)
                    VALUES (?, ?, ?, ?, ?, ?, ?)
                    """,
                    (device_id, datetime.utcnow().isoformat(), battery_voltage, battery_percent, wifi_rssi, free_heap, uptime_ms),
                )
                await db.commit()
                return
        except sqlite3.OperationalError as e:
            if "database is locked" in str(e) and attempt < MAX_RETRIES - 1:
                logger.warning(f"Database locked in record_heartbeat, retrying ({attempt + 1}/{MAX_RETRIES})...")
                await asyncio.sleep(RETRY_DELAY_S * (attempt + 1))
            else:
                raise


async def get_latest_heartbeat(device_id: str) -> Optional[dict]:
    """Get the most recent heartbeat for a device."""
    async with aiosqlite.connect(settings.database_path, timeout=BUSY_TIMEOUT_MS / 1000) as db:
        await db.execute("PRAGMA journal_mode=WAL")
        db.row_factory = aiosqlite.Row
        async with db.execute(
            "SELECT * FROM heartbeats WHERE device_id = ? ORDER BY timestamp DESC LIMIT 1",
            (device_id,)
        ) as cursor:
            row = await cursor.fetchone()
            return dict(row) if row else None





