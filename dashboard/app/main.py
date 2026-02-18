# uvicorn app.main:app --host 0.0.0.0 --port 8000
# python -m uvicorn app.main:app --host 0.0.0.0 --port 8000

from fastapi import FastAPI, Request
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.templating import Jinja2Templates
from fastapi.staticfiles import StaticFiles
from pathlib import Path
import json

BASE_DIR = Path(__file__).resolve().parent
DATA_FILE = BASE_DIR.parent.parent / "server" / "dados.txt"

app = FastAPI(title="Dashboard IoT")

app.mount("/static", StaticFiles(directory=str(BASE_DIR / "templates" / "static")), name="static")

templates = Jinja2Templates(directory=str(BASE_DIR / "templates"))

def read_last_snapshot():
    """
    Lê apenas a última linha do arquivo.
    Cada linha é um array completo de devices.
    """

    if not DATA_FILE.exists():
        return []

    last_line = None

    with open(DATA_FILE, "r", encoding="utf-8") as f:
        for line in f:
            if line.strip():
                last_line = line.strip()

    if not last_line:
        return []

    try:
        data = json.loads(last_line)

        # garante ordenação
        for device in data:
            device["Dados"] = sorted(
                device.get("Dados", []),
                key=lambda x: x.get("I", 0)
            )

        return sorted(data, key=lambda x: x.get("ID", ""))

    except json.JSONDecodeError:
        return []


@app.get("/", response_class=HTMLResponse)
def dashboard(request: Request):
    return templates.TemplateResponse(
        "index.html",
        {"request": request}
    )


@app.get("/api/data")
def api_data():
    devices = read_last_snapshot()
    return JSONResponse(devices)

@app.get("/api/history/channel/{device_id}/{channel_index}")
def get_channel_history(device_id: str, channel_index: int):
    if not DATA_FILE.exists():
        return JSONResponse({"timestamps": [], "tensao": [], "temp": [], "umi": []})

    history = {
        "timestamps": [],
        "tensao": [],
        "temp": [],
        "umi": []
    }

    with open(DATA_FILE, "r", encoding="utf-8") as f:
        for line in f:
            if not line.strip(): continue
            try:
                snapshot = json.loads(line)
                for device in snapshot:
                    if device.get("ID") == device_id:
                        dados = device.get("Dados", [])
                        for d in dados:
                            if d.get("I") == channel_index:
                                history["timestamps"].append(d.get("timestamp"))
                                history["tensao"].append(d.get("Tensao"))
                                history["temp"].append(d.get("Temp"))
                                history["umi"].append(d.get("Umi"))
            except json.JSONDecodeError:
                continue

    return JSONResponse(history)

@app.get("/api/history/{device_id}/{start}/{end}")
def get_history(device_id: str, start: int, end: int):

    if not DATA_FILE.exists():
        return JSONResponse({"timestamps": [], "canais": {}})

    timestamps = []
    canais = {str(i): [] for i in range(start, end + 1)}

    with open(DATA_FILE, "r", encoding="utf-8") as f:
        for line in f:
            if not line.strip():
                continue

            try:
                snapshot = json.loads(line)

                for device in snapshot:
                    if device.get("ID") == device_id:

                        dados = device.get("Dados", [])

                        # pega timestamp do primeiro canal (todos têm o mesmo)
                        if dados:
                            timestamps.append(dados[0]["timestamp"])

                        for d in dados:
                            canal = d.get("I")
                            if start <= canal <= end:
                                canais[str(canal)].append(d.get("Tensao"))

            except json.JSONDecodeError:
                continue

    return JSONResponse({
        "timestamps": timestamps,
        "canais": canais
    })