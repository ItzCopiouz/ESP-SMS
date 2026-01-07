"""
Heartbeat/telemetry endpoint for ESP32-D0WD-V3.

Receives periodic health updates from devices including:
- Battery voltage and percentage
- WiFi signal strength 
- Free heap memory
- Uptime since boot
"""

from fastapi import APIRouter, Header, HTTPException
from pydantic import BaseModel
from typing import Optional
import logging

from app.core.db import record_heartbeat, get_latest_heartbeat
from app.services.notify_pushover import send_low_battery_alert

router = APIRouter()
logger = logging.getLogger(__name__)

# Battery threshold for alerts (percentage)
LOW_BATTERY_THRESHOLD = 20


class HeartbeatPayload(BaseModel):
    """Telemetry data from ESP32-CAM device."""
    battery_voltage: Optional[int] = None  # millivolts
    battery_percent: Optional[int] = None  # 0-100
    wifi_rssi: Optional[int] = None  # dBm (typically -30 to -90)
    free_heap: Optional[int] = None  # bytes
    uptime_ms: Optional[int] = None  # milliseconds since boot


@router.post("/heartbeat")
async def receive_heartbeat(
    payload: HeartbeatPayload,
    x_device_id: str = Header(..., alias="X-Device-Id"),
):
    """
    Receive heartbeat/telemetry from an ESP32-CAM device.
    
    Records the data to the database and optionally triggers
    alerts for low battery conditions.
    """
    logger.info(
        f"Heartbeat from {x_device_id}: "
        f"battery={payload.battery_percent}% ({payload.battery_voltage}mV), "
        f"rssi={payload.wifi_rssi}dBm, heap={payload.free_heap}B, "
        f"uptime={payload.uptime_ms}ms"
    )
    
    # Record to database
    await record_heartbeat(
        device_id=x_device_id,
        battery_voltage=payload.battery_voltage,
        battery_percent=payload.battery_percent,
        wifi_rssi=payload.wifi_rssi,
        free_heap=payload.free_heap,
        uptime_ms=payload.uptime_ms,
    )
    
    # Check for low battery and send alert if needed
    if payload.battery_percent is not None and payload.battery_percent <= LOW_BATTERY_THRESHOLD:
        logger.warning(f"Low battery alert for {x_device_id}: {payload.battery_percent}%")
        send_low_battery_alert(x_device_id, payload.battery_percent, payload.battery_voltage)
    
    return {
        "ok": True,
        "device_id": x_device_id,
        "received": True,
    }


@router.get("/heartbeat/{device_id}")
async def get_device_status(device_id: str):
    """
    Get the latest heartbeat/status for a specific device.
    
    Useful for checking if a device is online and its battery status.
    """
    heartbeat = await get_latest_heartbeat(device_id)
    
    if not heartbeat:
        raise HTTPException(status_code=404, detail="No heartbeat found for this device")
    
    return heartbeat

