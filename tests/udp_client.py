import socket
import time

SERVER_IP = "127.0.0.1"
SERVER_PORT = 2053

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
iters  = 0

while True:

    message = f"{iters} >> hello from client {sock}"

    print(f"Sending: {message}")

    sock.sendto(message.encode(), (SERVER_IP, SERVER_PORT))

    data, addr = sock.recvfrom(4096)
    print(f"Received from {addr}: {data.decode()}")

    time.sleep(0.01)

sock.close()