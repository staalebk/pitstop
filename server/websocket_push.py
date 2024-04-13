import asyncio
from websockets.server import serve
import json
from main import active_senders


async def push_data_to_websocket(websocket):
    print("New websocket connection from", websocket.remote_address)

    # Find newest available timestamp in active_senders
    optimal_timestamp = max(sender['timestamp'] for sender in active_senders.values())

    # Subtract 5 seconds to the millisecond timestamp found above
    optimal_timestamp -= 5000

    # Find the closest timestamp to the optimal timestamp
    current_timestamp = 0
    for sender in active_senders.values():
        for entry in sender:
            if abs(current_timestamp - entry['timestamp']) < abs(current_timestamp - optimal_timestamp):
                current_timestamp = entry['timestamp']

    print("Starting to send data to", websocket.remote_address, "from timestamp", current_timestamp)

    while True:
        payload_to_send = []

        # Get each entry from active_senders that match current_timestamp
        for sender in active_senders.values():
            for entry in sender:
                if entry['timestamp'] == current_timestamp:
                    payload_to_send.append(entry)

        # Send the data over the websocket
        websocket.send(json.dumps(payload_to_send))
        print("Sent timestamp", current_timestamp, "to", websocket.remote_address, "with", len(payload_to_send), "entries")

        # Wait for a short period before sending the next update
        await asyncio.sleep(0.1)
        current_timestamp += 100


async def start_server(hostname, port):
    print("Listening to websocket", hostname + ":" + str(port))

    stop = asyncio.Future()
    async with serve(push_data_to_websocket, hostname, port):
        await stop
