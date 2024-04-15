import asyncio
import json
import time

import websockets
from websockets.server import serve
from shared import active_senders


# Helper function to get the current time in milliseconds
def current_milli_time():
    return int(round(time.time() * 1000))


# Add a space as thousand separator in the timestamp
def format_timestamp(time_num):
    return "{:,}".format(time_num).replace(",", " ")


async def push_data_to_websocket(websocket):
    print("New websocket connection from", websocket.remote_address)

    optimal_timestamp = 0
    # Find newest available timestamp in active_senders
    for sender_buffer in active_senders.values():
        for entry in sender_buffer:
            if entry['timestamp'] > optimal_timestamp:
                optimal_timestamp = entry['timestamp']

    # Subtract 5 seconds to the millisecond timestamp found above
    optimal_timestamp -= 5000

    print("Optimal timestamp to start at is", format_timestamp(optimal_timestamp), "ms")

    # Compare optimal timestamp to current server time to determine approximate latency
    # server_timestamp = current_milli_time()
    # print("Approximate latency is", format_timestamp(server_timestamp - optimal_timestamp), "ms")

    # Find the closest timestamp to the optimal timestamp
    current_timestamp = None
    for sender_buffer in active_senders.values():
        for entry in sender_buffer:
            if current_timestamp is None or abs(current_timestamp - entry['timestamp']) > abs(current_timestamp - optimal_timestamp):
                current_timestamp = entry['timestamp']

    print("Starting to send data to", websocket.remote_address, "from timestamp", format_timestamp(current_timestamp))

    while True:
        if websocket.closed:
            print("Websocket connection closed to", websocket.remote_address)
            break

        payload_to_send = []

        # Get each entry from active_senders that match current_timestamp
        for sender_uuid, sender_buffer in active_senders.items():
            for entry in sender_buffer:
                if entry['timestamp'] == current_timestamp:
                    payload_to_send.append(entry)

        print("Timestamp to send is", format_timestamp(current_timestamp))
        if len(payload_to_send) > 0:
            try:
                await websocket.send(json.dumps(payload_to_send))
                print("Sent", len(payload_to_send), "entries to", websocket.remote_address)
            except websockets.ConnectionClosedError:
                print("Websocket connection closed to", websocket.remote_address)
                break

        # Wait for a short period before sending the next update
        await asyncio.sleep(0.1)
        current_timestamp += 100


async def listen_for_websocket(hostname, port):
    print("Listening to websocket", hostname + ":" + str(port))

    stop = asyncio.Future()
    async with serve(push_data_to_websocket, hostname, port):
        await stop
