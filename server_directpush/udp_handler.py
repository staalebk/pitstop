import asyncio
import logging
import json
from typing import Dict, List, Tuple
from packet_parser import PacketParser, VehicleData, PacketData

# Configure logging
logging.basicConfig(
    level=logging.INFO, format="%(asctime)s - %(levelname)s - %(message)s"
)


# --- UDP Server Protocol ---
class UDPServerProtocol(asyncio.DatagramProtocol):
    def __init__(self, handler: "UDPHandler"):
        self.handler = handler
        self.transport = None

    def connection_made(self, transport: asyncio.DatagramTransport):
        self.transport = transport
        logging.info("UDP Endpoint opened")

    def datagram_received(self, data: bytes, addr: Tuple[str, int]):
        """Handles incoming datagrams."""
        # Use asyncio.create_task to handle processing asynchronously
        # without blocking the datagram_received method.
        asyncio.create_task(self.handler.process_datagram(data, addr))

    def error_received(self, exc: Exception):
        logging.error(f"UDP Error received: {exc}")

    def connection_lost(self, exc: Exception):
        logging.info("UDP Endpoint closed")


# --- UDP Handler Class ---
class UDPHandler:
    def __init__(self):
        self.last_timestamp: Dict[Tuple[str, int], int] = (
            {}
        )  # Store last timestamp per client
        self.authenticated_clients: Dict[Tuple[str, int], str] = {}
        self.websocket_server = None  # Initialize websocket_server attribute
        self.udp_transport = None  # Keep track of the UDP transport

    # Method to set WebSocket server instance
    def set_websocket_server(self, ws_server):
        self.websocket_server = ws_server
        logging.info("WebSocket server instance set in UDPHandler")

    async def start_server(self, host="127.0.0.1", port=5005):
        """Starts the UDP server."""
        loop = asyncio.get_running_loop()
        logging.info(f"Starting UDP server on {host}:{port}")
        try:
            # Create the datagram endpoint and store the transport
            transport, protocol = await loop.create_datagram_endpoint(
                lambda: UDPServerProtocol(self), local_addr=(host, port)
            )
            self.udp_transport = transport  # Store the transport
            logging.info("UDP server running.")
            # Removed await asyncio.Future() to allow concurrent execution
        except OSError as e:
            logging.error(f"Failed to bind UDP server to {host}:{port}: {e}")
            raise  # Re-raise to indicate failure
        except Exception as e:
            logging.error(f"Error starting UDP server: {e}")
            raise  # Re-raise to indicate failure

    def stop_server(self):
        """Stops the UDP server gracefully."""
        if self.udp_transport:
            logging.info("Stopping UDP server...")
            self.udp_transport.close()
            self.udp_transport = None
            logging.info("UDP server stopped.")
        else:
            logging.info("UDP server is not running.")

    async def process_datagram(self, data: bytes, addr: Tuple[str, int]):
        """Processes a received datagram after basic validation."""
        if not data:
            logging.warning(f"Received empty packet from {addr}")
            return

        logging.debug("Received packet from {addr}")

        packet_type = data[0]
        payload = data[1:]

        if packet_type == 0xFF:  # Authentication packet
            await self._handle_auth_packet(payload, addr)
        elif packet_type == 0x00:  # Data packet (version 0)
            await self._handle_data_packet(payload, addr)
        else:
            logging.warning(
                f"Received unknown packet type {packet_type:#04x} from {addr}"
            )

    async def _handle_auth_packet(self, payload: bytes, addr: Tuple[str, int]):
        """Handles authentication packets."""
        uuid = PacketParser.parse_auth_packet(payload)
        if uuid:
            self.authenticated_clients[addr] = uuid
            logging.info(f"Client {addr} authenticated with UUID: {uuid}")
            # Initialize last timestamp for this client
            self.last_timestamp[addr] = 0
        else:
            logging.warning(f"Authentication failed for client {addr}")

    async def _handle_data_packet(self, payload: bytes, addr: Tuple[str, int]):
        """Handles data packets (protocol v0)."""
        if addr not in self.authenticated_clients:
            logging.warning(f"Received data packet from unauthenticated client {addr}")
            return  # Ignore unauthenticated clients

        last_ts = self.last_timestamp.get(addr, 0)
        parsed_data = PacketParser.parse_data_packet(payload, True, last_ts)

        if not parsed_data:
            return  # Parsing failed or old packet

        timestamp, vehicle_data = parsed_data
        self.last_timestamp[addr] = timestamp

        logging.debug(f"Received Vehicle Data from {addr}: {vehicle_data}")

        # --- Process or Broadcast Data ---
        # 1. Send data to the WebSocket clients
        if self.websocket_server:
            # Create the packet data structure
            full_data: PacketData = {
                "uuid": self.authenticated_clients[addr],
                "vehicle": vehicle_data,
            }
            # Use create_task to avoid blocking datagram processing while broadcasting
            asyncio.create_task(self.websocket_server.broadcast(json.dumps(full_data)))
        # 2. Insert data into a database (Placeholder)
        # await self.insert_into_db(vehicle_data)
        logging.info(
            f"Successfully processed data packet from {addr} (UUID: {self.authenticated_clients.get(addr, 'N/A')}) with timestamp {timestamp}"
        )
