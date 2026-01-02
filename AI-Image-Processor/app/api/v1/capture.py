from fastapi import APIRouter, Request, Header, HTTPException

from app.config import settings
from app.core.db import create_job
from app.core.storage import generate_job_id, save_image

router = APIRouter()


@router.post("/capture")
async def capture_image(
    request: Request,
    x_device_id: str = Header(..., alias="X-Device-Id"),
):
    # Read JPEG bytes from request body
    image_data = await request.body()

    # Did we get anything?
    if not image_data:
        raise HTTPException(status_code=400, detail="No image data received")

    # Generate unique job id and save img
    job_id = generate_job_id(x_device_id)
    image_path = await save_image(job_id, image_data)

    # Record in db
    await create_job(job_id, x_device_id, str(image_path))

    # Return success response
    return {
        "ok": True,
        "job_id": job_id,
        "status": "queued",
        "result_url": f"{settings.base_url}/r/{job_id}",
    }

