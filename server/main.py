import socket
import asyncio
from udp_handler import handle_packet
from websocket_push import start_server

LISTEN_IP = "127.0.0.1"
UDP_PORT = 5005
WEBSOCKET_PORT = 8888

# Contains the last 5 seconds of data for each sender, in 100 ms increments to be
# used as a buffer for sending to websocket clients
active_senders = {
    "00000000-0000-0000-0000-000000000000": [
        {
            'timestamp': 1713047495700,
            'speed': 50,
            'heading': 90,
            'brake_temp_left': [],
            'brake_temp_right': [],
            'position': (0, 0)
        },
        {
            'timestamp': 1713047495600,
            'position': (0, 0)
        },
        {
            'timestamp': 1713047495500,
            'speed': 100,
            'heading': 90,
            'brake_temp_left': [],
            'brake_temp_right': [],
            'position': (0, 0)
        }
    ]
}

# Contains a mapping from the address of the sender to the UUID of the sender
addr_to_uuid = {}

if __name__ == "__main__":
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((LISTEN_IP, UDP_PORT))

    asyncio.run(start_server(LISTEN_IP, WEBSOCKET_PORT))

    while True:
        data, addr = sock.recvfrom(1024)
        handle_packet(data, addr[0] + ":" + addr[1])
