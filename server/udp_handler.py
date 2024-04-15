import struct
import uuid
import asyncudp
from collections import deque
from shared import active_senders

# Contains a mapping from the address of the sender to the UUID of the sender
addr_to_uuid = {}


def parse_data_packet(payload):
    try:
        # Unpack timestamp
        timestamp, = struct.unpack('<Q', payload[1:9])
        payload = payload[9:]

        # Change timestamps from microseconds to milliseconds
        timestamp = timestamp // 1000

        # Round to closest 100 ms
        timestamp = (timestamp // 100) * 100
        print("Data parsing timestamp", timestamp)

        # Unpack current data
        speed, heading, *brake_temp = struct.unpack('<HH' + 'H'*16, payload[:36])
        payload = payload[36:]
        print("Data parsing speed", speed)
        print("Data parsing heading", heading)
        print("Data parsing brake_temp", brake_temp)

        # Unpack array of the position of the last 40 * 100ms
        positions = []
        rpms = []
        for _ in range(40):
            lat, long, rpm = struct.unpack('<ffH', payload[:10])
            positions.append((lat, long))
            rpms.append(rpm)
            payload = payload[10:]

        print("Data parsing positions", positions)
        print("Data parsing rpm", rpms)

        message = {
            'timestamp': timestamp,
            'speed': speed / 100,  # Convert to km/h
            'heading': heading / 100,  # Convert to degrees
            'brake_temp': [temp / 10 - 100 for temp in brake_temp],  # 16 brake temps to C
            'positions': positions,  # Locations
            'rpms': rpms  # RPMs
        }

        return message

    except struct.error as e:
        print("Error parsing message:", e)
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

    print("Received data from", sender_addr, "containing timestamp ", data_message['timestamp'])

    if active_senders.get(sender_uuid) is None:
        active_senders[sender_uuid] = deque(maxlen=100)

    sender_buffer = active_senders[sender_uuid]

    # Add the newest data coming in with the packet
    new_sender_entry = {
        'id': sender_uuid,
        'timestamp': data_message['timestamp'],
        'speed': data_message['speed'],
        'heading': data_message['heading'],
        'brake_temp': data_message['brake_temp'],
        'position': data_message['positions'][0],
        'rpm': data_message['rpms'][0]
    }

    sender_buffer.append(new_sender_entry)

    position_and_rpm = zip(data_message['positions'][1:], data_message['rpms'][1:])

    # Update any missing positions in older timestamps in sender's buffer
    for index, (position, rpm) in enumerate(position_and_rpm):
        timestamp_for_entry = data_message['timestamp'] + ((index + 1) * 100)

        # If timestamp already exists in buffer, we just skip it
        if any(entry['timestamp'] == timestamp_for_entry for entry in sender_buffer):
            continue

        print("Backfilling",timestamp_for_entry, "with", position, "and", rpm)

        buffer_entry = {
            'id': sender_uuid,
            'timestamp': timestamp_for_entry,
            'position': position,
            'rpm': rpm
        }
        sender_buffer.appendleft(buffer_entry)

    # Sort buffer based on timestamp
    active_senders[sender_uuid] = deque(sorted(sender_buffer, key=lambda x: x['timestamp']), maxlen=100)

    print("Buffer ", sender_addr, "(", sender_uuid, ") contains", len(active_senders[sender_uuid]), "entries")


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
