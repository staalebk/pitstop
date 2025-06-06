import asyncio
import json
import websockets
import logging
import ssl
from typing import Set


class WebSocketServer:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.clients = set()
        self.server_instance = None  # Keep track of the server instance

    async def broadcast(self, message):
        if self.clients:
            await asyncio.wait(
                [asyncio.create_task(client.send(message)) for client in self.clients]
            )

    async def handler(self, websocket):
        """Simple handler that just adds the client to the set and removes it when disconnected.
        No longer processes incoming messages as they are not needed."""
        logging.info(f"Client connected: {websocket.remote_address}")

        self.clients.add(websocket)
        try:
            # Just keep the connection alive, but don't process incoming messages
            await websocket.wait_closed()
        except websockets.exceptions.ConnectionClosedOK:
            pass  # Client disconnected normally
        except websockets.exceptions.ConnectionClosedError as e:
            logging.error(f"WebSocket connection closed with error: {e}")
        finally:
            logging.info(f"Client disconnected: {websocket.remote_address}")
            self.clients.remove(websocket)

    async def start(self, devmode):
        """Starts the WebSocket server and returns the Server instance."""
        try:
            if devmode is False:
                logging.info(
                    "Starting WebSocket server in production mode, with SSL enabled"
                )
                cert_folder = "/etc/letsencrypt/live/pitstop.driftfun.no"
                ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
                cert_file = cert_folder + "/fullchain.pem"
                key_file = cert_folder + "/privkey.pem"
                ssl_context.load_cert_chain(cert_file, key_file)
            else:
                logging.info(
                    "Starting WebSocket server in development mode, with SSL disabled"
                )
                ssl_context = None

            # Start the server
            self.server_instance = await websockets.serve(
                self.handler, self.host, self.port, ssl=ssl_context
            )

            if devmode:
                logging.info(
                    f"WebSocket server started on ws://{self.host}:{self.port}"
                )
            else:
                logging.info(
                    f"WebSocket server started on wss://{self.host}:{self.port}"
                )

            return (
                self.server_instance
            )  # Return the server object for graceful shutdown handling
        except Exception as e:
            logging.error(f"Failed to start WebSocket server: {e}")
            raise  # Re-raise exception to be handled by the caller

    async def stop(self):
        """Stops the WebSocket server gracefully."""
        if self.server_instance:
            logging.info("Stopping WebSocket server...")
            self.server_instance.close()
            await self.server_instance.wait_closed()
            self.server_instance = None
            logging.info("WebSocket server stopped.")
        else:
            logging.info("WebSocket server is not running.")
