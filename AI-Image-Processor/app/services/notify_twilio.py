import logging
from twilio.rest import Client

from app.config import settings

logger = logging.getLogger(__name__)


def is_twilio_configured() -> bool:
    """see if we have everything needed for twilio"""
    return all([
        settings.twilio_account_sid,
        settings.twilio_auth_token,
        settings.twilio_from_number,
        settings.twilio_to_number,
    ])

def send_notification(result_text: str, result_url: str) -> None:
    """
    send an sms with the results
    if twilio isn't set up, we just log it to the terminal
    """
    # don't let the text get too long     
    max_chars = 1500
    if len(result_text) > max_chars:
        truncated = result_text[:max_chars] + "..."
    else:
        truncated = result_text

    message_body = f"{truncated}\n\nfull results: {result_url}"

    if not is_twilio_configured():
        logger.info (f"[SMS FALLBACK] would have sent: {message_body}")
        return

    # send it via twilio
    client = Client(settings.twilio_account_sid, settings.twilio_auth_token)

    message = client.messages.create(
        body = message_body,
        from_=settings.twilio_from_number,
        to=settings.twilio_to_number,
    )

    logger.info (f"sms is on its way: {message.sid}")
