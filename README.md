# ESP-SMS

ESP32-CAM → OpenAI Vision → Pushover notification pipeline.

## What It Does

1. **ESP32-CAM setup** takes a photo and uploads it to your server
2. **FastAPI server** queues the job in SQLite
3. **Worker** sends the image to OpenAI Vision for analysis
4. **Pushover** sends the result as a notification to your phone


