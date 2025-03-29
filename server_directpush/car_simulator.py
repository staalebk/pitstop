import socket
import time
import struct
import uuid
import math
import random
from typing import List, Tuple, Optional


class CarSimulator:
    """
    Simulates a car driving around a circuit and sends telemetry data via UDP.
    """

    def __init__(
        self,
        server_ip: str = "127.0.0.1",
        server_port: int = 5005,
        uuid_seed: Optional[str] = None,
    ):
        # Server connection details
        self.server_ip = server_ip
        self.server_port = server_port
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        # Car identifier - use seed if provided, otherwise generate random UUID
        if uuid_seed:
            # Create a UUID based on the provided seed
            self.car_uuid = uuid.uuid5(uuid.NAMESPACE_DNS, uuid_seed).bytes
            print(f"Using seeded UUID: {self.car_uuid.hex()} (from seed: {uuid_seed})")
        else:
            self.car_uuid = uuid.uuid4().bytes
            print(f"Using random UUID: {self.car_uuid.hex()}")

        # Circuit parameters (simple oval track)
        self.track_points = self._generate_track()
        self.current_point_index = 0

        # Car physics constants
        self.MAX_SPEED = 180.0  # km/h
        self.MAX_RPM = 6800  # max engine RPM
        self.MAX_ACCELERATION = 8.0  # m/s^2
        self.MAX_DECELERATION = 12.0  # m/s^2
        self.MAX_CORNERING_SPEED_FACTOR = 0.4  # Speed multiplier for sharpest turns
        self.LOOK_AHEAD_POINTS = 5  # Number of points to look ahead for turns
        self.CORNER_ANGLE_THRESHOLD_SHARP = 45  # Degrees - sharp turn threshold
        self.CORNER_ANGLE_THRESHOLD_MODERATE = 20  # Degrees - moderate turn threshold
        self.SPEED_SMOOTHING = 0.1  # Speed change smoothing factor (0-1)
        self.BRAKE_RESPONSE_RATE = 0.2  # Brake change rate (0-1)
        self.ACCELERATOR_RESPONSE_RATE = 0.15  # Accelerator change rate (0-1)
        self.DISTANCE_BETWEEN_POINTS = 10.0  # Approximate meters between track points

        # Initialize physics state
        self.target_speed = 0.0  # Target speed based on upcoming corners
        self.current_acceleration = 0.0  # Current acceleration/deceleration in m/s^2
        self.speed_ms = 0.0  # Speed in meters per second
        self.distance_along_segment = 0.0  # Distance traveled along current segment

        # Car state
        self.timestamp = int(time.time() * 1000000)  # Current timestamp in microseconds
        self.speed = 0.0  # km/h
        self.heading = 0.0  # degrees
        self.brake_temps = [60.0] * 16  # 16 brake temperature sensors
        self.heart_rate = 80  # driver's heart rate
        self.coolant_temp = 85  # engine coolant temperature
        self.oil_temp = 90  # engine oil temperature
        self.accelerator = 0  # 0-100%
        self.clutch = 0  # 0-100%
        self.brake = 0  # 0-100%
        self.latitude = self.track_points[0][0]  # current latitude
        self.longitude = self.track_points[0][1]  # current longitude
        self.rpm = 0  # engine RPM

        # Historical position data (40 entries with 100ms time difference)
        self.position_history = []
        for _ in range(40):
            self.position_history.append((self.latitude, self.longitude, self.rpm))

        # Authentication tracking
        self.last_auth_time = 0  # Last time authentication packet was sent

        # Simulation parameters
        self.authenticated = False

    def _generate_track(self) -> List[Tuple[float, float]]:
        """
        Generate an oval track with realistic GPS coordinates.
        Returns a list of (latitude, longitude) points.
        """
        # Center point (can be adjusted to any location)
        track_points = [
            (58.36926, 15.28448),
            (58.36975, 15.2865),
            (58.36999, 15.28703),
            (58.37018, 15.28733),
            (58.37053, 15.28747),
            (58.37081, 15.28742),
            (58.37104, 15.2872),
            (58.37123, 15.2869),
            (58.37147, 15.28602),
            (58.37166, 15.28534),
            (58.37178, 15.28509),
            (58.37186, 15.28499),
            (58.37201, 15.28491),
            (58.37213, 15.28493),
            (58.37223, 15.28499),
            (58.37233, 15.28507),
            (58.37242, 15.28518),
            (58.37256, 15.2855),
            (58.37261, 15.28568),
            (58.37262, 15.28588),
            (58.37259, 15.28614),
            (58.37254, 15.28638),
            (58.37243, 15.28685),
            (58.37222, 15.28752),
            (58.37211, 15.28779),
            (58.37197, 15.28808),
            (58.37189, 15.28821),
            (58.3718, 15.28832),
            (58.3716, 15.28842),
            (58.37138, 15.28847),
            (58.37056, 15.28858),
            (58.37045, 15.2886),
            (58.3703, 15.28877),
            (58.37018, 15.28885),
            (58.37013, 15.28884),
            (58.37007, 15.28882),
            (58.3698, 15.28831),
            (58.36931, 15.28729),
            (58.36621, 15.27502),
            (58.36616, 15.27465),
            (58.36616, 15.27422),
            (58.36627, 15.27398),
            (58.36641, 15.27382),
            (58.36656, 15.27372),
            (58.36669, 15.2737),
            (58.36681, 15.27374),
            (58.36695, 15.27382),
            (58.36721, 15.27425),
            (58.36745, 15.27476),
            (58.36851, 15.27789),
            (58.36872, 15.27829),
            (58.36899, 15.27858),
            (58.36934, 15.27869),
            (58.36974, 15.27866),
            (58.37029, 15.27862),
            (58.37035, 15.27867),
            (58.37039, 15.27877),
            (58.37041, 15.27894),
            (58.3704, 15.27916),
            (58.37037, 15.27937),
            (58.37032, 15.27965),
            (58.37024, 15.27987),
            (58.37012, 15.28009),
            (58.37, 15.28025),
            (58.36993, 15.28032),
            (58.36985, 15.28033),
            (58.36967, 15.28032),
            (58.36912, 15.28019),
            (58.36857, 15.28001),
            (58.36849, 15.28002),
            (58.36842, 15.28007),
            (58.36836, 15.28013),
            (58.36831, 15.28025),
            (58.3683, 15.28036),
            (58.3683, 15.28047),
        ]

        return track_points

    def authenticate(self):
        """Send authentication packet to the server."""
        # Packet format: 0xFF (auth packet type) + 16 bytes UUID
        auth_packet = bytes([0xFF]) + self.car_uuid
        self.socket.sendto(auth_packet, (self.server_ip, self.server_port))
        self.authenticated = True
        print(f"Sent authentication packet with UUID: {self.car_uuid.hex()}")

    def update_car_state(self):
        """Update the car's state for the next simulation step."""
        # Time delta (100ms as that's our update interval)
        dt = 0.1

        # Calculate the distance traveled in this time step
        distance_traveled = self.speed_ms * dt

        # Update position along track
        self._update_position(distance_traveled)

        # Look ahead for upcoming corners
        max_safe_speed = self._calculate_safe_speed()

        # Update target speed based on safe speed
        self.target_speed = min(self.MAX_SPEED, max_safe_speed)

        # Convert target_speed from km/h to m/s for calculations
        target_speed_ms = self.target_speed / 3.6

        # Determine if we need to accelerate or brake
        speed_diff = target_speed_ms - self.speed_ms

        if speed_diff > 0:
            # Accelerate
            self.current_acceleration = min(self.MAX_ACCELERATION, speed_diff / dt)
            self.brake = 0
            self.accelerator = min(
                100, (self.current_acceleration / self.MAX_ACCELERATION) * 100
            )
        else:
            # Brake
            self.current_acceleration = max(-self.MAX_DECELERATION, speed_diff / dt)
            self.accelerator = 0
            self.brake = min(
                100, (-self.current_acceleration / self.MAX_DECELERATION) * 100
            )

        # Update speed based on acceleration
        self.speed_ms = max(0, self.speed_ms + self.current_acceleration * dt)

        # Convert m/s to km/h for the car's speed property
        self.speed = self.speed_ms * 3.6

        # Update RPM based on speed (simplified model)
        self.rpm = int((self.speed / self.MAX_SPEED) * self.MAX_RPM)

        # Update temperatures and other state
        self._update_temperatures()
        self._update_driver_state()

        # Update timestamp
        current_time_us = int(time.time() * 1000000)
        self.timestamp = current_time_us - (current_time_us % 100000)

        # Update position history
        self.position_history.insert(0, (self.latitude, self.longitude, self.rpm))
        if len(self.position_history) > 40:
            self.position_history.pop()

    def _update_position(self, distance_traveled):
        """Update car position based on distance traveled."""
        while distance_traveled > 0:
            next_point_index = (self.current_point_index + 1) % len(self.track_points)
            next_point = self.track_points[next_point_index]
            current_point = self.track_points[self.current_point_index]

            # Calculate distance to next point
            dx = next_point[1] - current_point[1]
            dy = next_point[0] - current_point[0]
            segment_length = (
                math.sqrt(dx * dx + dy * dy) * 111000
            )  # Convert to meters (approximate)

            remaining_segment = segment_length - self.distance_along_segment

            if distance_traveled >= remaining_segment:
                # Move to next point
                self.current_point_index = next_point_index
                distance_traveled -= remaining_segment
                self.distance_along_segment = 0

                # Update heading
                self.heading = self._get_heading_at_index(self.current_point_index)
            else:
                # Partial movement along segment
                fraction = (
                    self.distance_along_segment + distance_traveled
                ) / segment_length
                self.latitude = current_point[0] + dy * fraction
                self.longitude = current_point[1] + dx * fraction
                self.distance_along_segment += distance_traveled
                distance_traveled = 0

    def _calculate_safe_speed(self):
        """Calculate safe speed based on upcoming track curvature."""
        max_angle_change = 0
        total_angle_change = 0

        # Look ahead several points to anticipate corners
        for i in range(self.LOOK_AHEAD_POINTS):
            idx = (self.current_point_index + i) % len(self.track_points)
            next_idx = (idx + 1) % len(self.track_points)
            angle_change = abs(
                self._get_heading_at_index(next_idx) - self._get_heading_at_index(idx)
            )

            # Normalize angle change to be between 0 and 180 degrees
            if angle_change > 180:
                angle_change = 360 - angle_change

            # Weight closer corners more heavily
            weighted_change = angle_change * (1.0 - (i / self.LOOK_AHEAD_POINTS) * 0.5)
            max_angle_change = max(max_angle_change, weighted_change)
            total_angle_change += weighted_change

        # Calculate safe speed based on maximum angle change
        if max_angle_change > self.CORNER_ANGLE_THRESHOLD_SHARP:
            safe_speed = self.MAX_SPEED * self.MAX_CORNERING_SPEED_FACTOR
        elif max_angle_change > self.CORNER_ANGLE_THRESHOLD_MODERATE:
            # Linear interpolation between moderate and sharp corner speeds
            factor = (max_angle_change - self.CORNER_ANGLE_THRESHOLD_MODERATE) / (
                self.CORNER_ANGLE_THRESHOLD_SHARP - self.CORNER_ANGLE_THRESHOLD_MODERATE
            )
            speed_factor = self.MAX_CORNERING_SPEED_FACTOR + (
                1 - self.MAX_CORNERING_SPEED_FACTOR
            ) * (1 - factor)
            safe_speed = self.MAX_SPEED * speed_factor
        else:
            # On straighter sections, allow full speed
            safe_speed = self.MAX_SPEED

        return safe_speed

    def _update_temperatures(self):
        """Update various temperature values based on car state."""
        # Update brake temperatures
        for i in range(16):
            if self.brake > 20:
                self.brake_temps[i] = min(
                    self.brake_temps[i] + random.uniform(0.1, 0.5) * (self.brake / 100),
                    800,
                )
            else:
                self.brake_temps[i] = max(
                    self.brake_temps[i] - random.uniform(0.1, 0.3), 60
                )

        # Engine temperatures increase with RPM
        rpm_factor = self.rpm / self.MAX_RPM
        self.coolant_temp = min(85 + int(rpm_factor * 15) + random.randint(-2, 2), 110)
        self.oil_temp = min(90 + int(rpm_factor * 20) + random.randint(-2, 2), 120)

    def _update_driver_state(self):
        """Update driver-related state (heart rate, clutch, etc.)."""
        # Heart rate increases with speed and cornering
        speed_stress = (self.speed / self.MAX_SPEED) * 40
        brake_stress = (self.brake / 100) * 20
        self.heart_rate = min(80 + int(speed_stress + brake_stress), 180)

        # Clutch simulation (occasional shifts)
        if random.random() < 0.05:  # 5% chance of shifting
            self.clutch = random.randint(50, 100)
        else:
            self.clutch = max(self.clutch - random.randint(5, 15), 0)

    def _get_heading_at_index(self, index: int) -> float:
        """Calculate heading at a specific track index."""
        current = self.track_points[index]
        next_idx = (index + 1) % len(self.track_points)
        next_point = self.track_points[next_idx]

        dx = next_point[1] - current[1]
        dy = next_point[0] - current[0]
        return (math.degrees(math.atan2(dy, dx)) + 360) % 360

    def send_telemetry(self):
        """Send telemetry data to the server."""
        if not self.authenticated:
            print("Cannot send telemetry: not authenticated")
            return

        # Packet format: 0x00 (data packet type) + payload
        # Payload format as per protocol in README.md:
        # - 8 bytes: timestamp (big-endian long) in microseconds
        # - 2 bytes: speed * 100 (big-endian unsigned short)
        # - 2 bytes: heading * 100 (big-endian unsigned short)
        # - 32 bytes: 16 brake temps, 0.1 C per bit, -100 C offset (big-endian unsigned short)
        # - 1 byte: heart rate
        # - 1 byte: coolant temp
        # - 1 byte: oil temp
        # - 1 byte: accelerator
        # - 1 byte: clutch
        # - 1 byte: brake
        # - 40 sets of positional data (each 10 bytes: lat, lon, rpm)

        # Pack timestamp (microseconds since 1970)
        payload = struct.pack("<q", self.timestamp)

        # Pack vehicle data
        payload += struct.pack("<H", int(self.speed * 100))
        payload += struct.pack("<H", int(self.heading * 100))

        # Pack brake temperatures
        for temp in self.brake_temps:
            # Convert to format expected by server: 0.1 C per bit, -100 C offset
            # Formula: (temp + 100) * 10
            adjusted_temp = int((temp + 100) * 10)
            payload += struct.pack("<H", adjusted_temp)

        # Pack other vehicle data
        payload += bytes(
            [
                self.heart_rate,
                self.coolant_temp,
                self.oil_temp,
                self.accelerator,
                self.clutch,
                self.brake,
            ]
        )

        # Pack positional data (40 entries with 100ms time difference)
        for lat, lon, rpm in self.position_history:
            lat_int = int(lat * 6000000)  # Convert to integer format
            lon_int = int(lon * 6000000)
            payload += struct.pack("<IIH", lat_int, lon_int, rpm)

        # Create and send the complete packet
        packet = bytes([0x00]) + payload
        self.socket.sendto(packet, (self.server_ip, self.server_port))

    def run_simulation(self, duration_seconds: int = 60, interval_ms: int = 100):
        """
        Run the simulation for the specified duration, sending packets at the given interval.

        Args:
            duration_seconds: How long to run the simulation (in seconds)
            interval_ms: Interval between packets (in milliseconds)
        """
        # First authenticate
        self.authenticate()
        self.last_auth_time = time.time()
        time.sleep(0.5)  # Wait a bit for authentication to be processed

        # Calculate number of iterations
        iterations = int(duration_seconds * 1000 / interval_ms)

        print(
            f"Starting simulation for {duration_seconds} seconds ({iterations} packets)..."
        )
        for i in range(iterations):
            start_time = time.time()

            # Check if it's time to re-authenticate (every 10 seconds)
            current_time = time.time()
            if current_time - self.last_auth_time >= 10:
                self.authenticate()
                self.last_auth_time = current_time

            # Update car state and send telemetry
            self.update_car_state()
            self.send_telemetry()

            # Calculate time to sleep to maintain the desired interval
            elapsed = (time.time() - start_time) * 1000  # in ms
            sleep_time = max(0, (interval_ms - elapsed) / 1000)  # in seconds

            if i % 10 == 0:  # Print status every 10 packets
                print(
                    f"Packet {i+1}/{iterations} - Speed: {self.speed:.1f} km/h, RPM: {self.rpm}, Position: ({self.latitude:.6f}, {self.longitude:.6f})"
                )

            time.sleep(sleep_time)

        print("Simulation completed!")


if __name__ == "__main__":
    import argparse

    # Set up command line argument parsing
    parser = argparse.ArgumentParser(description="Car telemetry simulator")
    parser.add_argument("--ip", default="127.0.0.1", help="Server IP address")
    parser.add_argument("--port", type=int, default=5005, help="Server port")
    parser.add_argument(
        "--duration", type=int, default=300, help="Simulation duration in seconds"
    )
    parser.add_argument(
        "--interval", type=int, default=100, help="Packet interval in milliseconds"
    )
    parser.add_argument("--seed", help="UUID seed for consistent car identification")

    args = parser.parse_args()

    # Create simulator with provided parameters
    simulator = CarSimulator(
        server_ip=args.ip, server_port=args.port, uuid_seed=args.seed
    )

    # Run simulation with provided duration and interval
    try:
        simulator.run_simulation(
            duration_seconds=args.duration, interval_ms=args.interval
        )
    except KeyboardInterrupt:
        print("Simulation stopped by user")
    except Exception as e:
        print(f"Error during simulation: {e}")
