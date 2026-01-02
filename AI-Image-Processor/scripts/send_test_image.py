#!/usr/bin/env python3
"""
Test script to upload an image to the API.
Usage: python scripts/send_test_image.py path/to/image.jpg
"""

import sys
import requests
from pathlib import Path

# Configuration
API_URL = "http://localhost:8000/api/v1/capture"
DEVICE_ID = "TEST_DEVICE"


def main():
    # Check command line arguments
    if len(sys.argv) < 2:
        print("Usage: python scripts/send_test_image.py <image_path>")
        print("Example: python scripts/send_test_image.py photo.jpg")
        sys.exit(1)

    image_path = Path(sys.argv[1])

    # Verify file exists
    if not image_path.exists():
        print(f"Error: File not found: {image_path}")
        sys.exit(1)

    # Read the image
    print(f"Reading image: {image_path}")
    image_data = image_path.read_bytes()
    print(f"Image size: {len(image_data)} bytes")

    # Send to API
    print(f"Uploading to {API_URL}...")
    response = requests.post(
        API_URL,
        headers={
            "X-Device-Id": DEVICE_ID,
            "Content-Type": "image/jpeg",
        },
        data=image_data,
    )

    # Print result
    print(f"Status: {response.status_code}")
    print(f"Response: {response.json()}")


if __name__ == "__main__":
    main()
