import struct
from datetime import datetime
import uuid
import asyncudp
from collections import deque
from shared import active_senders
from typing import TypedDict, List, Tuple

# Contains a mapping from the address of the sender to the UUID of the sender
addr_to_uuid = {}

class DataPacket(TypedDict):
    timestamp: int
    speed: float
    heading: float
    heartrate: int
    coolant_temp: int
    oil_temp: int
    accelerator: int
    clutch: int
    brake: int
    brake_temp: List[float]
    positions: List[Tuple[float, float]]
    rpms: List[int]

def parse_data_packet(payload) -> DataPacket | None:
    try:
        # Unpack protocol version
        protocol_version, = struct.unpack('<B', payload[0:1])
        if protocol_version != 0x00:
            raise ValueError("Unsupported protocol version")
        payload = payload[1:]

        # Unpack timestamp
        timestamp, = struct.unpack('<Q', payload[0:8])
        payload = payload[8:]

        # Change timestamps from microseconds to milliseconds
        timestamp = timestamp // 1000

        # Round to closest 100 ms
        timestamp = (timestamp // 100) * 100

        # Unpack current data
        speed, heading = struct.unpack('<HH', payload[0:4])
        payload = payload[4:]

        brake_temp = struct.unpack('<' + 'H'*16, payload[0:32])
        payload = payload[32:]

        heartrate, coolant_temp, oil_temp, accelerator, clutch, brake = struct.unpack('<BBBBBB', payload[0:6])
        payload = payload[6:]

        # Unpack array of the position of the last 40 * 100ms
        positions = []
        rpms = []
        for _ in range(40):
            lat, long, rpm = struct.unpack('<IIH', payload[0:10])
            lat /= 6000000.0
            long /= 6000000.0
            positions.append((lat, long))
            rpms.append(rpm)
            payload = payload[10:]

        message: DataPacket = {
            'timestamp': timestamp,
            'speed': speed / 100.0,  # Convert to km/h
            'heading': heading / 100.0,  # Convert to degrees
            'heartrate': heartrate,  # Heart rate
            'coolant_temp': coolant_temp,  # Coolant temperature
            'oil_temp': oil_temp,  # Engine oil temperature
            'accelerator': accelerator,  # Accelerator percentage
            'clutch': clutch,  # Clutch percentage
            'brake': brake,  # Brake percentage
            'brake_temp': [temp / 10 - 100 for temp in brake_temp],  # 16 brake temps to C
            'positions': positions,  # Locations
            'rpms': rpms  # RPMs
        }

        return message

    except struct.error as e:
        print("Error parsing message:", e)
        return None
    except ValueError as e:
        print("Error:", e)
        return None


def parse_auth_packet(payload):
    uuid_bytes = struct.unpack('16s', payload[1:])[0]
    packet_uuid = uuid.UUID(bytes=uuid_bytes)
    return packet_uuid


def is_auth_packet(packet):
    # An auth packet is 17 bytes long (128 bits), one byte for the packet type and 16 bytes for the UUID
    return packet[0] == 0xFF and len(packet) == 17


def handle_data_packet(payload, sender_addr):
    if sender_addr not in addr_to_uuid:
        print("Received data packet from unknown sender, waiting for auth packet for", sender_addr)
        return

    sender_uuid = addr_to_uuid[sender_addr]
    data_message = parse_data_packet(payload)

    if data_message is None:
        print("Received malformed data packet from", sender_addr)
        return

    # print("-------------- NEW PACKET --------------")
    # print("Received data from", sender_addr, "containing timestamp", data_message['timestamp'])

    if active_senders.get(sender_uuid) is None:
        active_senders[sender_uuid] = deque(maxlen=100)

    sender_buffer = active_senders[sender_uuid]

    # Add the newest data coming in with the packet
    new_sender_entry = {
        'id': str(sender_uuid),
        'timestamp': data_message['timestamp'],
        'speed': data_message['speed'],
        'heading': data_message['heading'],
        'coolant_temp': data_message['coolant_temp'],
        'oil_temp': data_message['oil_temp'],
        'accelerator': data_message['accelerator'],
        'clutch': data_message['clutch'],
        'brake': data_message['brake'],
        'position': data_message['positions'][0],
        'rpm': data_message['rpms'][0]
    }
    sender_buffer.appendleft(new_sender_entry)
    # print("Adding entry for timestamp", new_sender_entry['timestamp'])

    # Data received at clock time
    print("Data,", sender_addr, ",", datetime.now().time(),",", data_message['timestamp'],",", new_sender_entry['position'])

    position_and_rpm = zip(data_message['positions'][1:], data_message['rpms'][1:])

    # Update any missing positions in older timestamps in sender's buffer
    for index, (position, rpm) in enumerate(position_and_rpm):
        timestamp_for_entry = data_message['timestamp'] - ((index + 1) * 100)

        # If timestamp already exists in buffer, we just skip it
        if any(entry['timestamp'] == timestamp_for_entry for entry in sender_buffer):
            continue

        print("Backfilling", timestamp_for_entry, "with", position[0], "x", position[1], "and", rpm)

        buffer_entry = {
            'id': str(sender_uuid),
            'timestamp': timestamp_for_entry,
            'position': position,
            'rpm': rpm
        }
        sender_buffer.appendleft(buffer_entry)

    # Sort buffer based on timestamp
    active_senders[sender_uuid] = deque(reversed(sorted(sender_buffer, key=lambda x: x['timestamp'])), maxlen=100)

    if len(active_senders[sender_uuid]) > 0:
        # max_timestamp = max(timestamp['timestamp'] for timestamp in active_senders[sender_uuid])
        # min_timestamp = min(timestamp['timestamp'] for timestamp in active_senders[sender_uuid])
        # print("Buffer", sender_addr, "has", len(active_senders[sender_uuid]), "entries with timestamps from", min_timestamp, "to", max_timestamp)
        # print("Latest timestamp", active_senders[sender_uuid][0]['timestamp'])
        pass
    else:
        # print("Buffer", sender_addr, "is empty")
        pass


def handle_auth_packet(payload, sender):
    sender_uuid = parse_auth_packet(payload)
    print("Received auth from", sender, "containing uuid", sender_uuid)
    addr_to_uuid[sender] = sender_uuid


def handle_packet(payload, sender):
    if is_auth_packet(payload):
        handle_auth_packet(payload, sender)
    else:
        handle_data_packet(payload, sender)


async def listen_for_udp(hostname, port):
    sock = await asyncudp.create_socket(local_addr=(hostname, port))
    print("Listening to UDP on", hostname + ":" + str(port))

    while True:
        data, addr = await sock.recvfrom()
        handle_packet(data, addr[0] + ":" + str(addr[1]))
