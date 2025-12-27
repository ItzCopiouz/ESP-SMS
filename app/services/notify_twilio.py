import logging
from twilio.rest import Client

from app.config import settings

logger = logging.getLogger(__name__)


def is_twilio_configured() -> bool:
    """Check if Twilio is configured."""
    return all([
        settings.twilio_account_sid,
        settings.twilio_auth_token,
        settings.twilio_from_number,
        settings.twilio_to_number,
    ])

def send_notification(result_text: str, result_url: str) -> None:
    """
    Send SMS notification with result.
    Falls back to logging if Twilio not configured.
    """
    # Truncate text     
    max_chars = 1000
    if len(result_text) > max_chars:
        truncated = result_text[:max_chars] + "..."
    else:
        truncated = result_text

    message_body = f"{truncated}\n\nFull result: {result_url}"



    if not is_twilio_configured():
        logger.info (f"[SMS FALLBACK] Would send: {message_body}")
        return

    #send via Twilio
    client = Client(settings.twilio_account_sid, settings.twilio_auth_token)

    message = client.messages.create(
        body = message_body,
        from_=settings.twilio_from_number,
        to=settings.twilio_to_number,
    )

    logger.info (f"SMS sent: {message.sid}")


