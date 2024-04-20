import asyncio
import json
import math
import time
import ssl
from collections import deque

import websockets
from websockets.server import serve
from shared import active_senders


# Helper function to get the current time in milliseconds
def current_milli_time():
    return int(round(time.time() * 1000))


# Add a space as thousand separator in the timestamp
def format_timestamp(time_num):
    return time_num


def get_start_timestamp():
    optimal_timestamp = 0
    # Find newest available timestamp in active_senders
    for sender_buffer in active_senders.values():
        for entry in sender_buffer:
            if entry['timestamp'] > optimal_timestamp:
                optimal_timestamp = entry['timestamp']

    # Subtract 5 seconds to the millisecond timestamp found above
    optimal_timestamp -= 5000

    # print("Optimal timestamp to start at is", format_timestamp(optimal_timestamp), "ms")

    # Compare optimal timestamp to current server time to determine approximate latency
    # server_timestamp = current_milli_time()
    # print("Approximate latency is", format_timestamp(server_timestamp - optimal_timestamp), "ms")

    # Find the closest timestamp to the optimal timestamp
    current_timestamp = None
    for sender_buffer in active_senders.values():
        for entry in sender_buffer:
            if (current_timestamp is None or
                    abs(current_timestamp - entry['timestamp']) > abs(current_timestamp - optimal_timestamp)):
                current_timestamp = entry['timestamp']

    return current_timestamp


async def push_data_to_websocket(websocket):
    print("New websocket connection from", websocket.remote_address)

    current_timestamp = get_start_timestamp()
    print("Starting to send data to", websocket.remote_address, "from timestamp", format_timestamp(current_timestamp))

    # Debug map
    # lat = 59.3293
    # current_timestamp = 1000
    # active_senders['abc-123'] = deque([], maxlen=100)

    while True:
        if websocket.closed:
            print("Websocket connection closed to", websocket.remote_address)
            break

        if current_timestamp is None:
            current_timestamp = get_start_timestamp()

        # lat += 0.00001
        payload_to_send = []

        # Debug map
        # active_senders['abc-123'].appendleft({
        #     'id': 'abc-123',
        #     'timestamp': current_timestamp,
        #     'speed': 50 + math.sin(current_timestamp / 800) * 30,
        #     'heading': 50 + math.sin(current_timestamp / 800) * 30,
        #     'heartrate': 50 + math.sin(current_timestamp / 800) * 30,
        #     'brake_temp': [50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200],
        #     'position': (lat, 18.0686),
        #     'rpm': 3500 + math.sin(current_timestamp / 1000) * 2000
        # })

        # Get each entry from active_senders that match current_timestamp
        for sender_buffer in active_senders.values():
            for entry in sender_buffer:
                if entry['timestamp'] == current_timestamp:
                    payload_to_send.append(entry)

        if current_timestamp is not None:
            print("Looped, timestamp is", format_timestamp(current_timestamp))

        if len(payload_to_send) > 0:
            try:
                # print("Sending", payload_to_send)
                await websocket.send(json.dumps(payload_to_send))
                # print("Sent", len(payload_to_send), "entries to", websocket.remote_address)
            except websockets.ConnectionClosedError:
                print("Websocket connection closed to", websocket.remote_address)
                break

        # Wait for a short period before sending the next update
        await asyncio.sleep(0.1)
        if current_timestamp is not None:
            current_timestamp += 100


async def listen_for_websocket(hostname, port, dev=False):
    print("Listening to websocket", hostname + ":" + str(port))

    if dev is False:
        cert_folder = "../certs"
        ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        cert_file = cert_folder + "/fullchain.pem"
        key_file = cert_folder + "/privkey.pem"
        ssl_context.load_cert_chain(cert_file, key_file)
    else:
        ssl_context = None

    stop = asyncio.Future()
    async with serve(push_data_to_websocket, hostname, port, ssl=ssl_context):
        await stop
