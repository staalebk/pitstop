import argparse
import asyncio
import time

from udp_handler import listen_for_udp
from websocket_push import listen_for_websocket

udp_port = 5005
websocket_port = 8888

async def main():
    parser = argparse.ArgumentParser(description='Start the server with optional SSL for websocket.')
    parser.add_argument('--dev', action='store_true', help='Skip SSL and use localhost for sockets')
    args = parser.parse_args()

    print("Starting Pitstop server v. 0.2 - " + time.asctime())
    if args.dev:
        print("Running in dev mode")
        listen_ip = "192.168.1.110"
    else:
        print("Running in production mode")
        listen_ip = "pitstop.driftfun.no"

    udp_task = asyncio.create_task(listen_for_udp(listen_ip, udp_port))
    websocket_task = asyncio.create_task(listen_for_websocket(listen_ip, websocket_port, args.dev))

    await asyncio.gather(udp_task, websocket_task)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("Server stopped by user")