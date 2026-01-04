import base64
import json
import os
from pathlib import Path

from dotenv import load_dotenv
from openai import OpenAI

from app.config import settings

# reload .env so it overrides anything in the shell
load_dotenv(override=True)


def load_system_prompt() -> str:
    """grab the system prompt from its file"""
    path = settings.system_prompt_path
    if path.exists():
        return path.read_text().strip()
    return "Describe what you see in this image. Be concise and to the point."


def analyze_image(image_path: Path) -> dict:
    """
    send the photo to openai vision and see what it says
    returns a dict with the raw response and the clean text
    """
    # encode the image so we can send it
    image_bytes = image_path.read_bytes()
    base64_image = base64.b64encode(image_bytes).decode("utf-8")

    # setup the client and prompt
    api_key = os.environ.get("OPENAI_API_KEY", settings.openai_api_key)
    client = OpenAI(api_key=api_key)
    system_prompt = load_system_prompt()

    # hit up the vision api
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

    # grab just the text part
    text = response.choices[0].message.content

    # return the whole thing and the text
    return {
        "raw_response": response.model_dump(),
        "text": text,
    }
