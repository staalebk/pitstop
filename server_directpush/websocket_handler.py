import asyncio
import json
import websockets
import logging
from typing import Set


class WebSocketServer:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.clients = set()
        self.server_instance = None  # Keep track of the server instance

    async def broadcast(self, message):
        if self.clients:
            await asyncio.wait([asyncio.create_task(client.send(message)) for client in self.clients])

    async def handler(self, websocket):
        """Simple handler that just adds the client to the set and removes it when disconnected.
        No longer processes incoming messages as they are not needed."""
        self.clients.add(websocket)
        try:
            # Just keep the connection alive, but don't process incoming messages
            await websocket.wait_closed()
        except websockets.exceptions.ConnectionClosedOK:
            pass  # Client disconnected normally
        except websockets.exceptions.ConnectionClosedError as e:
            logging.error(f"WebSocket connection closed with error: {e}")
        finally:
            self.clients.remove(websocket)

    async def start(self):
        """Starts the WebSocket server and returns the Server instance."""
        try:
            # Start the server
            self.server_instance = await websockets.serve(
                self.handler, self.host, self.port
            )
            logging.info(f"WebSocket server started on ws://{self.host}:{self.port}")
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
