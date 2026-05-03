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
    image_data = await request.body()

    if not image_data:
        raise HTTPException(status_code=400, detail="No image data received")

    job_id = generate_job_id(x_device_id)
    image_path = await save_image(job_id, image_data)

    await create_job(job_id, x_device_id, str(image_path))

    return {
        "ok": True,
        "job_id": job_id,
        "status": "queued",
        "result_url": f"{settings.base_url}/api/v1/results/{job_id}",
    }
