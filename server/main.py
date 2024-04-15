import asyncio
from udp_handler import listen_for_udp
from websocket_push import listen_for_websocket

LISTEN_IP = "192.168.1.110"
UDP_PORT = 5005
WEBSOCKET_PORT = 8888


if __name__ == "__main__":
    loop = asyncio.get_event_loop()

    try:
        udp_task = loop.create_task(listen_for_udp(LISTEN_IP, UDP_PORT))
        websocket_task = loop.create_task(listen_for_websocket(LISTEN_IP, WEBSOCKET_PORT))
        loop.run_until_complete(asyncio.gather(udp_task, websocket_task))

    except KeyboardInterrupt:
        pass
    finally:
        if loop.is_running():
            loop.close()
