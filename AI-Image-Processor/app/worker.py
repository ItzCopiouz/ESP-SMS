import json
import time
import logging
from pathlib import Path

from app.config import settings
from app.core.db import claim_next_job, complete_job, fail_job
from app.services.openai_vision import analyze_image
from app.services.notify_twilio import send_notification

# setup logging so we know what the worker is up to
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
)
logger = logging.getLogger(__name__)

def get_results_dir() -> Path:
    """make sure the results folder exists"""
    results_dir = settings.data_dir / "results"
    results_dir.mkdir(parents=True, exist_ok=True)
    return results_dir

def process_job(job: dict) -> None:
    """actually do the work: ask the ai, save the stuff, send the sms"""
    job_id = job["job_id"]
    image_path = Path(job["image_path"])

    logger.info(f"doing work on: {job_id}")

    try:
        # ask openai vision what's in the photo
        result = analyze_image(image_path)

        # save the results so we don't lose 'em
        results_dir = get_results_dir()

        json_path = results_dir / f"{job_id}.json"
        txt_path = results_dir / f"{job_id}.txt"

        # save the raw json from openai
        with open(json_path, "w") as f:
            json.dump(result["raw_response"], f, indent=4)

        # save the clean text
        with open(txt_path, "w") as f:
            f.write(result["text"])

        # mark it as done in the database
        complete_job(job_id, str(json_path), str(txt_path))

        # hit up twilio to send the text
        result_url = f"{settings.base_url}/api/v1/results/{job_id}"
        send_notification(result["text"], result_url)

        logger.info(f"job {job_id} finished without a hitch")

    except Exception as e:
        logger.error(f"it broke: {str(e)}")
        fail_job(job_id, str(e))


def run_worker() -> None:
    """the main loop that keeps looking for work"""
    while True:
        # see if there's anything to do
        job = claim_next_job()
        if job:
            process_job(job)
        else:
            # nothing to do, chill for a bit
            time.sleep(2)


if __name__ == "__main__":
    run_worker()
