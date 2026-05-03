import base64
import json
import os
from pathlib import Path

from dotenv import load_dotenv
from openai import OpenAI

from app.config import settings

# Force reload .env to override any shell environment variables
load_dotenv(override=True)


def load_system_prompt() -> str:
    """Load the system prompt from the file."""
    path = settings.system_prompt_path
    if path.exists():
        return path.read_text().strip()
    return "Describe what you see in this image. Be concise and to the point."


def analyze_image(image_path: Path) -> dict:
    """
    Send image to OpenAI Vision and get analysis.

    Returns dict with 'raw_response' (full JSON) and 'text' (extracted message).
    """
    image_bytes = image_path.read_bytes()
    base64_image = base64.b64encode(image_bytes).decode("utf-8")

    api_key = os.environ.get("OPENAI_API_KEY", settings.openai_api_key)
    client = OpenAI(api_key=api_key)
    system_prompt = load_system_prompt()

    response = client.chat.completions.create(
        model=settings.openai_model,
        messages=[
            {"role": "system", "content": system_prompt},
            {
                "role": "user",
                "content": [
                    {
                        "type": "image_url",
                        "image_url": {
                            "url": f"data:image/jpeg;base64,{base64_image}"
                        },
                    },
                ],
            },
        ],
        max_completion_tokens=2000,
    )

    text = response.choices[0].message.content

    return {
        "raw_response": response.model_dump(),
        "text": text,
    }
