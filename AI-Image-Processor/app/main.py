from fastapi import FastAPI
from contextlib import asynccontextmanager

from app.core.db import init_db
from app.api.v1 import capture, results

@asynccontextmanager
async def lifespan(app: FastAPI):
    init_db()
    print("Database initialized")

    yield 

    #shutdown
    print("Shutting down...")


app = FastAPI(
    title="ESP32-D0WD-V3 CAM Processor",
    lifespan=lifespan,
)

# setup the api routes
app.include_router(capture.router, prefix="/api/v1")
app.include_router(results.router, prefix="/api/v1")
