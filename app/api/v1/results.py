from fastapi import APIRouter, HTTPException

from app.core.db import get_job

router = APIRouter()


@router.get("/results/{job_id}")
async def get_result(job_id: str):
    """Get job status and results."""
    job = await get_job(job_id)

    if not job:
        raise HTTPException(status_code=404, detail="Job not found")

    return job
