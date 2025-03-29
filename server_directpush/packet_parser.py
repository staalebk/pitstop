import struct
import logging
from typing import Dict, List, Tuple, TypedDict, Optional


# --- TypedDict Definitions ---
class VehicleData(TypedDict):
    timestamp: int
    speed: float
    heading: float
    brake_temps: List[float]
    heart_rate: int
    coolant_temp: int
    oil_temp: int
    accelerator: int
    clutch: int
    brake: int
    latitude: float
    longitude: float
    rpm: int


class PacketData(TypedDict):
    uuid: str
    vehicle: VehicleData


class PacketParser:
    """
    Handles parsing of UDP packets for the Pitstop telemetry system.
    This class is responsible for extracting structured data from raw bytes.
    """

    @staticmethod
    def parse_auth_packet(payload: bytes) -> Optional[str]:
        """
        Parse an authentication packet.

        Args:
            payload: Raw bytes containing the authentication data

        Returns:
            UUID string if valid, None otherwise
        """
        if len(payload) == 16:  # 128-bit UUID
            return payload.hex()
        else:
            logging.warning(f"Received invalid auth packet length {len(payload)}")
            return None

    @staticmethod
    def parse_data_packet(
        payload: bytes, timestamp_check: bool = True, last_timestamp: int = 0
    ) -> Optional[Tuple[int, VehicleData]]:
        """
        Parse a data packet (protocol v0).

        Args:
            payload: Raw bytes containing the telemetry data
            timestamp_check: Whether to check if the timestamp is newer than last_timestamp
            last_timestamp: The last received timestamp for comparison

        Returns:
            Tuple of (timestamp, vehicle_data) if valid, None otherwise
            Note: Only the first position and RPM data is used, the rest are discarded
        """
        # Check minimum length (8 byte timestamp + vehicle data + at least one position)
        # Vehicle data: 2+2+32+1+1+1+1+1+1 = 42 bytes
        # Position data: 4+4+2 = 10 bytes
        min_len = 8 + 42 + 10
        if len(payload) < min_len:
            logging.warning(f"Received short data packet ({len(payload)} bytes)")
            return None

        try:
            # --- Parse Timestamp ---
            timestamp = struct.unpack("<q", payload[:8])[0]

            # Check if this is an old packet
            if timestamp_check and timestamp <= last_timestamp:
                logging.debug(
                    f"Ignoring old packet (timestamp: {timestamp} <= last: {last_timestamp})"
                )
                return None

            # --- Parse Vehicle Data (offset 8) ---
            vehicle_data: VehicleData = {
                "timestamp": timestamp,
                "speed": struct.unpack("<H", payload[8:10])[0] / 100.0,
                "heading": struct.unpack("<H", payload[10:12])[0] / 100.0,
                "brake_temps": [
                    t * 0.1 - 100.0 for t in struct.unpack("<16H", payload[12:44])
                ],
                "heart_rate": payload[44],
                "coolant_temp": payload[45],
                "oil_temp": payload[46],
                "accelerator": payload[47],
                "clutch": payload[48],
                "brake": payload[49],
                # Initialize with default values
                "latitude": 0.0,
                "longitude": 0.0,
                "rpm": 0,
            }

            # --- Parse Positional Data (offset 50) ---
            pos_data_bytes = payload[50:]

            # Only process the first position data if available
            if len(pos_data_bytes) >= 10:  # Each position record is 10 bytes
                lat_raw, lon_raw, rpm = struct.unpack("<IIH", pos_data_bytes[:10])
                # Add position and RPM directly to vehicle data
                vehicle_data["latitude"] = lat_raw / 6000000.0
                vehicle_data["longitude"] = lon_raw / 6000000.0
                vehicle_data["rpm"] = rpm

                # Log if we're discarding additional position data
                if len(pos_data_bytes) > 10:
                    num_discarded = (len(pos_data_bytes) - 10) // 10
                    logging.debug(
                        f"Discarding {num_discarded} additional position records as requested"
                    )

            return timestamp, vehicle_data

        except struct.error as e:
            logging.error(f"Failed to unpack data packet: {e}")
        except IndexError as e:
            logging.error(f"Data packet seems incomplete: {e}")
        except Exception as e:
            logging.error(f"Unexpected error processing data packet: {e}")

        return None
