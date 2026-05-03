from contextlib import asynccontextmanager
import logging

from fastapi import FastAPI

from app.core.db import init_db
from app.api.v1 import capture, results, heartbeat

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
)
logger = logging.getLogger(__name__)


@asynccontextmanager
async def lifespan(app: FastAPI):
    init_db()
    logger.info("Database initialized with WAL mode")

    yield

    logger.info("Shutting down...")


app = FastAPI(
    title="ESP32-D0WD-V3 CAM Processor",
    lifespan=lifespan,
)

# Routers
app.include_router(capture.router, prefix="/api/v1")
app.include_router(results.router, prefix="/api/v1")
app.include_router(heartbeat.router, prefix="/api/v1")
