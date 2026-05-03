from pathlib import Path
from typing import Optional

from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    model_config = SettingsConfigDict(
        env_file=".env",
        env_file_encoding="utf-8",
        extra="ignore",
    )

    # Core paths
    base_url: str = "http://localhost:8000"
    data_dir: Path = Path("./data")
    database_path: Path = Path("./data/app.db")

    # OpenAI Vision settings
    openai_api_key: str
    openai_model: str = "gpt-5.2-2025-12-11"
    system_prompt_path: Path = Path("./prompts/system.txt")

    # Pushover settings
    pushover_user_key: Optional[str] = None
    pushover_api_token: Optional[str] = None


settings = Settings()
