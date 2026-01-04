import logging
import requests
import time
from typing import Optional

from app.config import settings

logger = logging.getLogger(__name__)

# Track last alert time per device to avoid spam
_last_low_battery_alert: dict[str, float] = {}
LOW_BATTERY_ALERT_COOLDOWN_SECONDS = 3600  # 1 hour between alerts per device


def is_pushover_configured() -> bool:
    """Check if Pushover is configured."""
    return all([
        settings.pushover_user_key,
        settings.pushover_api_token,
    ])


def _send_message(message_body: str, title: Optional[str] = None, url: Optional[str] = None, url_title: Optional[str] = None, priority: int = 0) -> bool:
    """Internal helper to send message via Pushover."""
    if not is_pushover_configured():
        logger.info(f"[PUSHOVER FALLBACK] Would send: {title} - {message_body}")
        return False

    try:
        payload = {
            "token": settings.pushover_api_token,
            "user": settings.pushover_user_key,
            "message": message_body,
            "priority": priority,
        }
        
        if title:
            payload["title"] = title
            
        if url:
            payload["url"] = url
            
        if url_title:
            payload["url_title"] = url_title

        response = requests.post("https://api.pushover.net/1/messages.json", data=payload, timeout=10)
        response.raise_for_status()
        
        logger.info(f"Pushover notification sent: {title}")
        return True
    except Exception as e:
        logger.error(f"Failed to send Pushover notification: {e}")
        return False


def send_notification(result_text: str, result_url: str) -> None:
    """
    Send notification with result.
    Falls back to logging if Pushover not configured.
    """
    # Truncate text if needed (Pushover limit is 1024 chars, but we want to save space for other fields)
    max_chars = 900
    if len(result_text) > max_chars:
        truncated = result_text[:max_chars] + "..."
    else:
        truncated = result_text

    _send_message(
        message_body=truncated,
        title="New Analysis Result",
        url=result_url,
        url_title="View Full Result"
    )


def send_low_battery_alert(device_id: str, battery_percent: int, battery_mv: Optional[int] = None) -> None:
    """
    Send low battery alert for a device.
    Includes cooldown to prevent alert spam.
    """
    
    # Check cooldown
    now = time.time()
    last_alert = _last_low_battery_alert.get(device_id, 0)
    
    if now - last_alert < LOW_BATTERY_ALERT_COOLDOWN_SECONDS:
        logger.debug(f"Skipping low battery alert for {device_id} (cooldown)")
        return
    
    # Build message
    voltage_str = f" ({battery_mv}mV)" if battery_mv else ""
    message_body = (
        f"Device: {device_id}\n"
        f"Battery: {battery_percent}%{voltage_str}\n\n"
        f"Please charge or replace the battery soon."
    )
    
    if _send_message(message_body, title="⚠️ LOW BATTERY ALERT", priority=1):
        _last_low_battery_alert[device_id] = now
        logger.info(f"Low battery alert sent for {device_id}")


def send_device_offline_alert(device_id: str, last_seen: str) -> None:
    """
    Send alert when a device hasn't checked in for a while.
    """
    message_body = (
        f"Device: {device_id}\n"
        f"Last seen: {last_seen}\n\n"
        f"The device may be powered off or experiencing issues."
    )
    _send_message(message_body, title="⚠️ DEVICE OFFLINE", priority=1)

