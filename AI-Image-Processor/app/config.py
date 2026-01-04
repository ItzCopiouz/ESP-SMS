from pathlib import Path
from pydantic_settings import BaseSettings, SettingsConfigDict
from typing import Optional


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

    # Sam Retardman settings
    openai_api_key: str
    openai_model: str = "gpt-5.2-2025-12-11"
    system_prompt_path: Path = Path("./prompts/system.txt")

    # twilio info (optional, will just log to console if missing)
    twilio_account_sid: Optional[str] = None
    twilio_auth_token: Optional[str] = None
    twilio_from_number: Optional[str] = None
    twilio_to_number: Optional[str] = None


# Singleton (me fr)
settings = Settings()
