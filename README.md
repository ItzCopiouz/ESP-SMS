# ESP-SMS

ESP32-CAM image capture pipeline with a FastAPI backend, OpenAI Vision analysis,
SQLite job tracking, and Pushover notifications.

## What It Does

1. An ESP32-CAM wakes from deep sleep, connects to WiFi, captures a JPEG, and
   posts it to the backend.
2. The FastAPI service saves the image and creates a queued job in SQLite.
3. A background worker claims queued jobs, sends images to OpenAI Vision, and
   stores the response.
4. Pushover sends the analysis result to a phone. Device heartbeat telemetry is
   also stored and can trigger low-battery alerts.

## Repository Layout

```text
app/                 FastAPI backend, SQLite helpers, worker, notifications
firmware/            ESP-IDF project for the ESP32-CAM
prompts/             System prompt used for image analysis
scripts/             Local utility scripts
Dockerfile           Backend image for API and worker containers
docker-compose.yml   Local deployment for API, worker, and persistent data
```

## Backend Setup

Copy the environment template and fill in your values:

```bash
cp .env.example .env
```

Required:

- `OPENAI_API_KEY`

Optional:

- `BASE_URL`
- `OPENAI_MODEL`
- `PUSHOVER_USER_KEY`
- `PUSHOVER_API_TOKEN`

Run the backend with Docker Compose:

```bash
docker compose up --build
```

The API is exposed on `127.0.0.1:8001` by default. The capture endpoint is:

```text
POST http://127.0.0.1:8001/api/v1/capture
```

Requests must include an `X-Device-Id` header and a JPEG request body.

## Firmware Setup

1. Install ESP-IDF 5.x.
2. Copy the WiFi credentials template:

   ```bash
   cp firmware/main/wifi_secrets.h.example firmware/main/wifi_secrets.h
   ```

3. Edit `firmware/main/wifi_secrets.h` with one or more WiFi networks.
4. Configure the backend URL and device ID:

   ```bash
   cd firmware
   idf.py menuconfig
   ```

5. Build and flash:

   ```bash
   idf.py build
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

## API Endpoints

- `POST /api/v1/capture`: accepts a JPEG and queues image analysis.
- `GET /api/v1/results/{job_id}`: returns job status and result paths.
- `POST /api/v1/heartbeat`: records device telemetry.
- `GET /api/v1/heartbeat/{device_id}`: returns the latest heartbeat for a device.

## Local Test Upload

```bash
python scripts/send_test_image.py path/to/photo.jpg
```

