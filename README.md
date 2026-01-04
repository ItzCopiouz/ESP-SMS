# ESP-SMS

ESP32-CAM → OpenAI Vision → Pushover notification pipeline.

## What It Does

1. **ESP32-CAM setup** takes a photo and uploads it to your server
2. **FastAPI server** queues the job in SQLite
3. **Worker** sends the image to OpenAI Vision for analysis
4. **Pushover** sends the result as a notification to your phone

## Quick Start

```bash
# Clone and setup
git clone https://github.com/YOUR_USERNAME/ESP-SMS.git
cd ESP-SMS
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt

# Configure
cp .env.example .env
# Edit .env with your OpenAI API key (and optionally Pushover)

# Run (two terminals)
uvicorn app.main:app --host 0.0.0.0 --port 8000  # Terminal 1
python -m app.worker                              # Terminal 2

# Test
python scripts/send_test_image.py path/to/image.jpg
```

## Docker Deployment

```bash
# On your server
cp .env.example .env
# Edit .env with your keys

docker compose up -d
```

## Environment Variables

| Variable | Required | Description |
|----------|----------|-------------|
| `OPENAI_API_KEY` | Yes | Your OpenAI API key |
| `OPENAI_MODEL` | No | Default: `gpt-5.2` |
| `PUSHOVER_USER_KEY` | No | For Pushover notifications |
| `PUSHOVER_API_TOKEN` | No | For Pushover notifications |

## API Endpoints

- `POST /api/v1/capture` - Upload image (raw JPEG bytes, `X-Device-Id` header)
- `GET /api/v1/results/{job_id}` - Get job status and results

## Repo Layout

- **firmware/**: ESP-IDF firmware for ESP32-CAM
- **AI-Image-Processor/**: Python FastAPI backend + OpenAI integration

## Firmware Setup

1. Navigate to `firmware/`
2. Configure: `idf.py menuconfig` (set WiFi + backend URL)
3. Flash: `idf.py flash monitor`

## Backend Setup

1. Navigate to `AI-Image-Processor/`
2. Install deps: `pip install -r requirements.txt`
3. Run: `python app/main.py`
