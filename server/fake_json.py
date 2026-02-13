import json
import random
import time
from pathlib import Path

DATA_FILE = Path(__file__).resolve().parent.parent / "server" / "dados.txt"


def generate_device(device_number):
    now = int(time.time())

    canais = []

    for i in range(1, 13):
        canais.append({
            "I": i,
            "Tensao": round(random.uniform(14.0, 21.0), 2),
            "Temp": random.randint(15, 40),
            "Umi": random.randint(30, 80),
            "timestamp": now
        })

    return {
        "ID": f"Device {device_number}",
        "Dados": canais
    }


def inject_snapshot(qtd_devices):
    snapshot = []

    for d in range(1, qtd_devices + 1):
        snapshot.append(generate_device(d))

    # adiciona como UMA linha nova
    with open(DATA_FILE, "a", encoding="utf-8") as f:
        f.write(json.dumps(snapshot) + "\n")

    print(f"\nSnapshot com {qtd_devices} device(s) adicionado!")
    print(f"Linha adicionada no arquivo.")


if __name__ == "__main__":

    # print("=== Injetor de Snapshot IoT ===")

    # qtd = int(input("Quantos devices deseja gerar neste snapshot? "))
    # inject_snapshot(qtd)
    while True:
        print("gerando novos dados...")
        inject_snapshot(1)
        time.sleep(5)