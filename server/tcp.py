import socket
import threading
import json
from datetime import datetime

HOST = '0.0.0.0'
PORT = 5000
OUTPUT_FILE = "server/dados.txt"

file_lock = threading.Lock()  # garante escrita segura entre threads


def handle_client(conn, addr):
    print(f"[+] Nova conexão: {addr}")
    buffer = ""

    try:
        while True:
            data = conn.recv(1024)
            if not data:
                break

            buffer += data.decode(errors="ignore")

            while "\n" in buffer:
                line, buffer = buffer.split("\n", 1)
                line = line.strip()

                if not line:
                    continue

                try:
                    payload = json.loads(line)

                    # ===== PARSE DOS CAMPOS =====
                    mac = payload["mac"]
                    ts = int(payload["timestamp"])

                    # valores vêm *100 do ESP
                    temp = int(payload["temperatura"]) / 100.0
                    umid = int(payload["umidade"]) / 100.0

                    dt = datetime.fromtimestamp(ts).strftime(
                        "%Y-%m-%d %H:%M:%S"
                    )

                    parsed = {
                        "mac": mac,
                        "timestamp": ts,
                        "datetime": dt,
                        "temperatura": round(temp, 2),
                        "umidade": round(umid, 2)
                    }

                    print(f"[{addr}] {parsed}")

                    # ===== PERSISTÊNCIA EM ARQUIVO =====
                    with file_lock:
                        with open(OUTPUT_FILE, "a", encoding="utf-8") as f:
                            f.write(json.dumps(parsed) + "\n")

                    # ===== ACK =====
                    conn.sendall(b"OK\n")

                except (json.JSONDecodeError, KeyError, ValueError) as e:
                    print(f"[{addr}] Erro de parsing:", e)
                    print("Linha:", line)

    except Exception as e:
        print(f"[{addr}] Erro:", e)

    finally:
        conn.close()
        print(f"[-] Conexão encerrada: {addr}")


def start_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, PORT))
    server.listen()

    print(f"Servidor TCP ouvindo em {HOST}:{PORT}")

    while True:
        conn, addr = server.accept()
        thread = threading.Thread(
            target=handle_client,
            args=(conn, addr),
            daemon=True
        )
        thread.start()


if __name__ == "__main__":
    start_server()