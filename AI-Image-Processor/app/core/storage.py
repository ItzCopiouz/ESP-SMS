import aiofiles
from pathlib import Path

from app.config import settings

import random
import string
from datetime import datetime, timezone

def generate_job_id(device_id: str) -> str:
    """Generate a unique job ID: YYYYMMDD_HHMMSSZ_{device}_{rand6}"""
    now = datetime.now(timezone.utc)
    timestamp = now.strftime("%Y%m%d_%H%M%SZ")
    rand = "".join(random.choices(string.ascii_lowercase + string.digits, k=6))
    return f"{timestamp}_{device_id}_{rand}"

def get_image_path(job_id: str) -> Path:
    """Get the path where an image should be saved."""
    images_dir = settings.data_dir / "images"
    images_dir.mkdir(parents=True, exist_ok=True)
    return images_dir / f"{job_id}.jpg"

async def save_image(job_id: str, data: bytes) -> Path:
    """Save image bytes to disk."""
    path = get_image_path(job_id)
    async with aiofiles.open(path, "wb") as f:
        await f.write(data)
    return path