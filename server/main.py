import argparse
import asyncio
import time

from udp_handler import listen_for_udp
from websocket_push import listen_for_websocket

UDP_PORT = 5005
WEBSOCKET_PORT = 8888

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Start the server with optional SSL for websocket.')
    parser.add_argument('--dev', action='store_true', help='Skip SSL and use localhost for sockets')
    args = parser.parse_args()

    print("Starting Pitstop server v. 0.2 - " + time.asctime())
    if args.dev:
        print("Running in dev mode")
        LISTEN_IP = "192.168.1.110"
    else:
        print("Running in production mode")
        LISTEN_IP = "pitstop.driftfun.no"

    loop = asyncio.get_event_loop()

    try:
        udp_task = loop.create_task(listen_for_udp(LISTEN_IP, UDP_PORT))
        websocket_task = loop.create_task(listen_for_websocket(LISTEN_IP, WEBSOCKET_PORT, args.dev))
        loop.run_until_complete(asyncio.gather(udp_task, websocket_task))

    except KeyboardInterrupt:
        pass
    finally:
        if loop.is_running():
            loop.close()
