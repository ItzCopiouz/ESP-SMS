#!/usr/bin/env python3
"""
Test script to upload an image to the API.
Usage: python scripts/send_test_image.py path/to/image.jpg
"""

import sys
import os
from pathlib import Path

import requests

API_URL = os.getenv("API_URL", "http://127.0.0.1:8001/api/v1/capture")
DEVICE_ID = os.getenv("DEVICE_ID", "TEST_DEVICE")


def main():
    if len(sys.argv) < 2:
        print("Usage: python scripts/send_test_image.py <image_path>")
        print("Example: python scripts/send_test_image.py photo.jpg")
        sys.exit(1)

    image_path = Path(sys.argv[1])

    if not image_path.exists():
        print(f"Error: File not found: {image_path}")
        sys.exit(1)

    print(f"Reading image: {image_path}")
    image_data = image_path.read_bytes()
    print(f"Image size: {len(image_data)} bytes")

    print(f"Uploading to {API_URL}...")
    try:
        response = requests.post(
            API_URL,
            headers={
                "X-Device-Id": DEVICE_ID,
                "Content-Type": "image/jpeg",
            },
            data=image_data,
            timeout=30,
        )

        print(f"Status: {response.status_code}")
        print(f"Response: {response.json()}")
    except Exception as e:
        print(f"Error sending request: {e}")


if __name__ == "__main__":
    main()
