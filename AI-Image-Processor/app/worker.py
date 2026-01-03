import json
import time
import logging
from pathlib import Path

from app.config import settings
from app.core.db import claim_next_job, complete_job, fail_job
from app.services.openai_vision import analyze_image
from app.services.notify_twilio import send_notification

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
        #vision
        result = analyze_image(image_path)

        #save results
        results_dir = get_results_dir()

        json_path = results_dir / f"{job_id}.json"
        txt_path = results_dir / f"{job_id}.txt"

        #save raw json
        with open(json_path, "w") as f:
            json.dump(result["raw_response"], f, indent=4)

        #save text
        with open(txt_path, "w") as f:
            f.write(result["text"])

        # Mark job as complete
        complete_job(job_id, str(json_path), str(txt_path))

        # Send notification
        result_url = f"{settings.base_url}/api/v1/results/{job_id}"
        send_notification(result["text"], result_url)

        logger.info(f"Job {job_id} completed successfully")

    except Exception as e:
        logger.error(f"Job {job_id} failed: {str(e)}")
        fail_job(job_id, str(e))


def run_worker() -> None:
    """Run the worker: claim jobs, process them, repeat."""
    while True:
        # Claim next job
        job = claim_next_job()
        if job:
            process_job(job)
        else:
            #wait before try
            time.sleep(2)


if __name__ == "__main__":
    run_worker()
    