import socket
import time
from dataclasses import dataclass
import struct

@dataclass
class DNSHeader:
    id: int
    qr: int = 0
    opcode: int = 0
    aa: int = 0
    tc: int = 0
    rd: int = 0
    ra: int = 0
    rz: int = 0
    rcode: int = 0
    qd_count: int = 0
    an_count: int = 0
    ns_count: int = 0
    ar_count: int = 0

    def to_bytes(self):
        flags = (
            (self.qr << 15) |
            (self.opcode << 11) |
            (self.aa << 10) |
            (self.tc << 9) |
            (self.rd << 8) |
            (self.ra << 7) |
            (self.rz << 4) |
            self.rcode
        )

        return struct.pack(
            "!HHHHHH",
            self.id,
            flags,
            self.qd_count,
            self.an_count,
            self.ns_count,
            self.ar_count,
        )

@dataclass
class DNSLabel:
    len : int = 0
    content : str = ""

    def to_bytes(self):
        self.content = self.content[:63].encode(encoding="ascii")

        return struct.pack(
            f"!B{self.len}s",
            self.len,
            self.content
        )


@dataclass
class DNSQuestion:
    content : str = ""
    qtype : int = 0
    qclass : int = 0


    def to_bytes( self ):

        labels = self.content.split('.')
        labels = [ 
            DNSLabel(
                len=len(label),
                content=label
            ) 
            for label in labels
        ]

        bytes = b"".join([i.to_bytes() for i in labels])
        bytes += b"\x00"

        bytes += struct.pack(
            "!HH",
            self.qtype,
            self.qclass
        )

        return bytes


SERVER_IP = "127.0.0.1"
SERVER_PORT = 2053

iters  = 0

with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:

    sock.settimeout(5)

    while True:

        message = DNSHeader(
            id = 1234,
            qr = 0,
            qd_count=1
        ).to_bytes()

        message += DNSQuestion(
            content="codecrafters.io",
            qtype=1,
            qclass=1
        ).to_bytes()

        print(f"Sending: {message} from {sock}")
        sock.sendto(message, (SERVER_IP, SERVER_PORT) )

        try:
            data, addr = sock.recvfrom(4096)
            print(f"Received from {addr}: '{data.hex()}'")
        except:
            print("Timeout occured")

        time.sleep(1)
        iters += 1
