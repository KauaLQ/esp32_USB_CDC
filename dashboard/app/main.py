# uvicorn app.main:app --host 0.0.0.0 --port 8000

from fastapi import FastAPI, Request
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.templating import Jinja2Templates
from pathlib import Path
import json

BASE_DIR = Path(__file__).resolve().parent
DATA_FILE = BASE_DIR.parent.parent / "server" / "dados.txt"

templates = Jinja2Templates(directory=str(BASE_DIR / "templates"))

app = FastAPI(title="Dashboard IoT")


def read_last_by_device():
    """
    Retorna um dict:
    { mac: ultima_leitura }
    """
    devices = {}

    if not DATA_FILE.exists():
        return devices

    with open(DATA_FILE, "r", encoding="utf-8") as f:
        for line in f:
            try:
                data = json.loads(line)
                devices[data["mac"]] = data  # mantém apenas a última leitura
            except json.JSONDecodeError:
                pass

    return devices


def chunk_devices(devices, chunk_size=6):
    """
    Quebra dispositivos em blocos de 6
    """
    # Ordena por MAC para manter ordem estável
    device_list = sorted(devices.values(), key=lambda x: x["mac"])

    chunks = []
    for i in range(0, len(device_list), chunk_size):
        chunks.append(device_list[i:i + chunk_size])

    return chunks


@app.get("/", response_class=HTMLResponse)
def dashboard(request: Request):
    return templates.TemplateResponse(
        "index.html",
        {"request": request}
    )


@app.get("/api/data")
def api_data():
    devices = read_last_by_device()
    tables = chunk_devices(devices)

    if not tables:
        tables = [[]]
        
    # for t in tables:
    #     for ta in t:
    #         print(ta)

    return JSONResponse(tables)