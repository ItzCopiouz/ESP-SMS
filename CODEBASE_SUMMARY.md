# Codebase Summary

## Purpose

ESP-SMS is an ESP32-CAM image analysis pipeline. A camera device wakes up,
captures a JPEG, uploads it to a FastAPI backend, and returns to deep sleep.
The backend queues the image, a worker sends it to OpenAI Vision, and Pushover
can notify a phone with the result.

## Main Components

- `firmware/`: ESP-IDF firmware for WiFi connection, camera capture, image
  upload, heartbeat telemetry, battery monitoring, and deep sleep.
- `app/main.py`: FastAPI app setup and router registration.
- `app/api/v1/capture.py`: accepts JPEG uploads and queues processing jobs.
- `app/api/v1/results.py`: returns queued, processing, done, or failed job data.
- `app/api/v1/heartbeat.py`: records device telemetry and triggers low-battery
  notifications.
- `app/core/db.py`: SQLite schema and job/heartbeat persistence helpers.
- `app/core/storage.py`: image path generation and async image writes.
- `app/worker.py`: polling worker that claims jobs, analyzes images, stores
  results, and sends notifications.
- `app/services/openai_vision.py`: OpenAI Vision request wrapper.
- `app/services/notify_pushover.py`: Pushover notification wrapper.

## Runtime Flow

1. ESP32-CAM boots or wakes from deep sleep.
2. Firmware initializes power management, WiFi, and camera.
3. Firmware posts a JPEG to `POST /api/v1/capture` with `X-Device-Id`.
4. Backend stores the image under `data/images/` and inserts a queued job.
5. Worker claims the next queued job and calls OpenAI Vision.
6. Worker stores JSON/text outputs under `data/results/`.
7. Worker marks the job done and sends a Pushover notification when configured.
8. Firmware posts telemetry to `POST /api/v1/heartbeat`.

## Cleanup Performed

- Removed the stale nested `AI-Image-Processor/` backend copy.
- Removed the committed `.env.save` file from the working tree.
- Added `.env.example` and tightened env-file ignore rules.
- Expanded `README.md` with setup, layout, endpoints, and security notes.
- Made the test upload script configurable with `API_URL` and `DEVICE_ID`.
- Cleaned casual comments/log text in backend and firmware files.

## Follow-Up Risks

- Rotate any real credentials that were ever committed in `.env.save`.
- Add device authentication before exposing the API directly to the internet.
- Consider adding automated tests for API endpoints and database transitions.
- Consider moving from polling SQLite worker logic to a queue if traffic grows.
