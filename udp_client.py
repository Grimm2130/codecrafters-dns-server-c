import socket

SERVER_IP = "127.0.0.1"
SERVER_PORT = 2053

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

message = f"hello from client {sock}"

print(f"Sending: {message}")

sock.sendto(message.encode(), (SERVER_IP, SERVER_PORT))

data, addr = sock.recvfrom(4096)
print(f"Received from {addr}: {data.decode()}")

sock.close()