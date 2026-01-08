# ESP-SMS

ESP32-CAM → OpenAI Vision → Pushover notification pipeline.

## Recent Updates

- **ESP-IDF 5.x Compatibility**: Updated code to comply with ESP-IDF 5.x standards so it would flash properly.

## What It Does

1. **ESP32-CAM setup** takes a photo and uploads it to your server
2. **FastAPI server** queues the job in SQLite
3. **Worker** sends the image to OpenAI Vision for analysis
4. **Pushover** sends the result as a notification to your phone

## Repo Layout (Canonical)

- **`firmware/`**: ESP-IDF firmware for ESP32-CAM
- **`app/`**: FastAPI backend + worker (this is what the root `Dockerfile` + `docker-compose.yml` run)
- **`AI-Image-Processor/`**: Legacy/old copy of the backend (not used for deployments; keep only if you need it for reference)

## Quick Start

```bash
# Clone and setup
git clone https://github.com/YOUR_USERNAME/ESP-SMS.git
cd ESP-SMS
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt

# Configure (create a .env file; it is gitignored)
nano .env
# Add at least OPENAI_API_KEY (and Pushover keys if you want notifications)

# Run (two terminals)
uvicorn app.main:app --host 0.0.0.0 --port 8000  # Terminal 1
python -m app.worker                              # Terminal 2

# Test
python scripts/send_test_image.py path/to/image.jpg
```

## Docker Deployment

```bash
# On your server
cd ~/ESP32CAM
nano .env
# Edit .env with your keys

docker compose up -d --build
```

## Environment Variables

| Variable | Required | Description |
|----------|----------|-------------|
| `OPENAI_API_KEY` | Yes | Your OpenAI API key |
| `OPENAI_MODEL` | No | Default: `gpt-5.2` |
| `BASE_URL` | No | Public base URL used for result links (recommended for notifications). Example: `https://esp32.samcc.work` |
| `PUSHOVER_USER_KEY` | No | For Pushover notifications |
| `PUSHOVER_API_TOKEN` | No | For Pushover notifications |

## API Endpoints

- `POST /api/v1/capture` - Upload image (raw JPEG bytes, `X-Device-Id` header)
- `GET /api/v1/results/{job_id}` - Get job status and results

## Repo Layout

See **Repo Layout (Canonical)** above.

## Firmware Setup

1. Navigate to `firmware/`
2. Configure: `idf.py menuconfig` (set WiFi + backend URL)
3. Flash: `idf.py flash monitor`

## Backend Setup

1. Navigate to the repo root: `cd ~/ESP32CAM`
2. Install deps: `pip install -r requirements.txt`
3. Run: `uvicorn app.main:app --host 0.0.0.0 --port 8000` and `python -m app.worker`
