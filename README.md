# ESP32-CAM AI Image Processor

This project captures images with an ESP32-CAM and processes them with a Python backend using OpenAI's Vision API.

## Structure

- **firmware/**: ESP-IDF C firmware for ESP32-D0WD-V3 + OV3660
- **AI-Image-Processor/**: Python FastAPI backend + OpenAI integration

## Getting Started

### Firmware
1. Navigate to `firmware/`
2. Configure: `idf.py menuconfig` (Set WiFi & Backend URL)
3. Flash: `idf.py flash monitor`

### Backend
1. Navigate to `AI-Image-Processor/`
2. Install deps: `pip install -r requirements.txt`
3. Run: `python main.py`

