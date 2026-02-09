import socket
import threading
import json

HOST = '0.0.0.0'
PORT = 5000


def handle_client(conn, addr):
    print(f"[+] Nova conexão: {addr}")
    buffer = ""

    try:
        while True:
            data = conn.recv(1024)
            if not data:
                break

            buffer += data.decode()

            while "\n" in buffer:
                line, buffer = buffer.split("\n", 1)
                line = line.strip()

                if not line:
                    continue

                try:
                    payload = json.loads(line)
                    print(f"[{addr}] {payload}")
                    conn.sendall(b"OK\n")
                except json.JSONDecodeError:
                    print(f"[{addr}] JSON inválido:", line)

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