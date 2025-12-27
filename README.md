# ESP-SMS

ESP32-CAM → OpenAI Vision → SMS notification pipeline.

## What It Does

1. **ESP32-CAM setup** takes a photo and uploads it to your server
2. **FastAPI server** queues the job in SQLite
3. **Worker** sends the image to OpenAI Vision for analysis
4. **Twilio** sends the result as SMS to your phone

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
# Edit .env with your OpenAI API key (and optionally Twilio)

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
| `TWILIO_ACCOUNT_SID` | No | For SMS notifications |
| `TWILIO_AUTH_TOKEN` | No | For SMS notifications |
| `TWILIO_FROM_NUMBER` | No | Your Twilio phone number |
| `TWILIO_TO_NUMBER` | No | Your personal phone number |

## API Endpoints

- `POST /api/v1/capture` - Upload image (raw JPEG bytes, `X-Device-Id` header)
- `GET /api/v1/results/{job_id}` - Get job status and results

## Project Structure

```
app/
  main.py              # FastAPI app
  config.py            # Settings from .env
  worker.py            # Background job processor
  core/
    db.py              # SQLite operations
    storage.py         # File I/O
  api/v1/
    capture.py         # Upload endpoint
    results.py         # Results endpoint
  services/
    openai_vision.py   # OpenAI Vision API
    notify_twilio.py   # SMS notifications
```

