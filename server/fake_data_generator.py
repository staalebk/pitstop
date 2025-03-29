import asyncio
import struct
import time
import random
import uuid
from udp_handler import handle_packet

async def send_fake_udp_data(hostname, port):
    print(f"Injecting fake UDP data from {hostname}:{port}")

    # Generate a UUID for the fake sender
    sender_uuid = uuid.uuid4()
    auth_packet = struct.pack('B', 0xFF) + sender_uuid.bytes

    # Send the auth packet
    handle_packet(auth_packet, f"{hostname}:{port}")
    print(f"Injected auth packet with UUID {sender_uuid}")

    while True:
        protocol_version = 0x00
        timestamp = int(time.time() * 1000000)  # microseconds
        speed = random.uniform(0, 200)  # km/h
        heading = random.uniform(0, 360)  # degrees
        brake_temp = [random.uniform(100, 500) for _ in range(16)]
        heartrate = random.randint(60, 180)
        coolant_temp = random.uniform(70, 120)
        oil_temp = random.uniform(70, 120)
        accelerator = random.uniform(0, 100)
        clutch = random.uniform(0, 100)
        brake = random.uniform(0, 100)
        positions = [(random.uniform(-90, 90), random.uniform(-180, 180)) for _ in range(40)]
        rpms = [random.randint(1000, 8000) for _ in range(40)]

        payload = struct.pack('<B', protocol_version)
        payload += struct.pack('<Q', timestamp)
        payload += struct.pack('<HH', int(speed * 100), int(heading * 100))
        payload += struct.pack('<' + 'H'*16, *[int(temp * 10 + 1000) for temp in brake_temp])
        payload += struct.pack('<BBBBBB', heartrate, int(coolant_temp), int(oil_temp), int(accelerator), int(clutch), int(brake))
        for lat, long in positions:
            payload += struct.pack('<IIH', int(lat * 6000000), int(long * 6000000), rpms.pop(0))

        # Inject the fake data packet directly into the app
        handle_packet(payload, f"{hostname}:{port}")
        print(f"Injected fake data packet with timestamp {timestamp // 1000} ms")

        await asyncio.sleep(random.uniform(0.5, 1.5))