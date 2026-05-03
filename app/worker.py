import json
import logging
import time
from pathlib import Path

from app.config import settings
from app.core.db import claim_next_job, complete_job, fail_job, init_db
from app.services.openai_vision import analyze_image
from app.services.notify_pushover import send_notification

# Logging setup
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
)
logger = logging.getLogger(__name__)


def get_results_dir() -> Path:
    """Get (and create) the results directory."""
    results_dir = settings.data_dir / "results"
    results_dir.mkdir(parents=True, exist_ok=True)
    return results_dir


def process_job(job: dict) -> None:
    """Process a single job: analyze image, save results, notify."""
    job_id = job["job_id"]
    image_path = Path(job["image_path"])

    logger.info(f"Processing job: {job_id}")

    try:
        result = analyze_image(image_path)
        results_dir = get_results_dir()

        json_path = results_dir / f"{job_id}.json"
        txt_path = results_dir / f"{job_id}.txt"

        with open(json_path, "w", encoding="utf-8") as f:
            json.dump(result["raw_response"], f, indent=4)

        with open(txt_path, "w", encoding="utf-8") as f:
            f.write(result["text"])

        complete_job(job_id, str(json_path), str(txt_path))

        result_url = f"{settings.base_url}/api/v1/results/{job_id}"
        send_notification(result["text"], result_url)

        logger.info(f"Job {job_id} completed successfully")

    except Exception as e:
        logger.error(f"Job {job_id} failed: {str(e)}")
        fail_job(job_id, str(e))


def run_worker() -> None:
    """Run the worker: claim jobs, process them, repeat."""
    # Ensure DB schema exists (avoids race if worker starts before API init)
    try:
        init_db()
    except Exception as e:
        logger.error(f"Worker failed to initialize database: {e}")
        raise

    logger.info("Worker started, watching for jobs...")
    while True:
        job = claim_next_job()
        if job:
            process_job(job)
        else:
            time.sleep(2)


if __name__ == "__main__":
    run_worker()
