import struct
import uuid
from collections import deque
from main import active_senders, addr_to_uuid


def parse_data_packet(payload):
    try:
        # Unpack timestamp
        timestamp, = struct.unpack('>Q', payload[:8])
        payload = payload[8:]

        # Change timestamps from microseconds to milliseconds
        timestamp = timestamp // 1000

        # Round to closest 100 ms
        timestamp = (timestamp // 100) * 100

        # Unpack current data
        speed, heading, *brake_temp = struct.unpack('>HH' + 'H'*16, payload[:36])
        payload = payload[36:]

        # Unpack array of the position of the last 40 * 100ms
        positions = []
        for _ in range(40):
            lat, long, rpm = struct.unpack('>ffH', payload[:10])
            positions.append((lat, long, rpm))
            payload = payload[10:]

        message = {
            'timestamp': timestamp,
            'speed': speed / 100,  # Convert to km/h
            'heading': heading / 100,  # Convert to degrees
            'brake_temp_left': [temp / 10 - 100 for temp in brake_temp[:16]],  # Left brake temps to C
            'brake_temp_right': [temp / 10 - 100 for temp in brake_temp[16:]],  # Right brake temps to C
            'positions': positions
        }

        return message

    except struct.error:
        print("Error parsing message")
        return None


def parse_auth_packet(payload):
    uuid_bytes = struct.unpack('16s', payload)[0]
    packet_uuid = uuid.UUID(bytes=uuid_bytes)
    return packet_uuid


def is_auth_packet(packet):
    # An auth packet is 16 bytes long (128 bits)
    return len(packet) == 16


def handle_data_packet(payload, sender_addr):
    if sender_addr not in addr_to_uuid:
        print("Received data packet from unknown sender, waiting for auth packet for", sender_addr)
        return

    sender_uuid = addr_to_uuid[sender_addr]
    data_message = parse_data_packet(payload)

    if active_senders[sender_uuid] is None:
        active_senders[sender_uuid] = deque(maxlen=100)

    sender_buffer = active_senders[sender_uuid]

    # Add the newest data coming in with the packet
    new_sender_entry = {
        'timestamp': data_message['timestamp'],
        'speed': data_message['speed'],
        'heading': data_message['heading'],
        'brake_temp_left': data_message['brake_temp_left'],
        'brake_temp_right': data_message['brake_temp_right'],
        'position': data_message['positions'][0]
    }

    sender_buffer.append(new_sender_entry)

    # Update any missing positions in older timestamps in sender's buffer
    for index, position in enumerate(data_message['positions'][1:]):
        timestamp_for_position = data_message['timestamp'] + ((index + 1) * 100)

        # If timestamp already exists in buffer, we just skip it
        if any(entry['timestamp'] == timestamp_for_position for entry in sender_buffer):
            continue

        buffer_entry = {
            'timestamp': timestamp_for_position,
            'position': position
        }
        sender_buffer.appendleft(buffer_entry)

    # Sort buffer based on timestamp
    active_senders[sender_uuid] = deque(sorted(sender_buffer, key=lambda x: x['timestamp']), maxlen=100)

    print("Buffer ", sender_addr, "(", sender_uuid, ") contains", len(active_senders[sender_uuid]), "entries")


def handle_auth_packet(payload, sender):
    sender_uuid = parse_auth_packet(payload)
    print("received auth from", sender, "containing uuid", sender_uuid)
    addr_to_uuid[sender] = sender_uuid


def handle_packet(payload, sender):
    print("received payload", payload, "from", sender)
    if is_auth_packet(payload):
        handle_auth_packet(payload, sender)
    else:
        handle_data_packet(payload, sender)