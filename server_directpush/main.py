import asyncio
import logging
import signal
import argparse
from udp_handler import UDPHandler
from websocket_handler import WebSocketServer

# Configure basic logging
logging.basicConfig(
    level=logging.INFO, format="%(asctime)s - %(levelname)s - %(message)s"
)

# Global flag to signal shutdown
shutdown_requested = asyncio.Event()


async def main():
    parser = argparse.ArgumentParser(description="Pitstop Server")
    parser.add_argument("--dev", action="store_true", help="Use development host")
    args = parser.parse_args()

    if args.dev:
        host = "192.168.1.110"
    else:
        host = "pitstop.driftfun.no"

    ws_port = 8888
    udp_port = 5005

    ws_server = WebSocketServer(host=host, port=ws_port)
    udp_handler = UDPHandler()

    # Link the UDP handler to the WebSocket server for broadcasting
    udp_handler.set_websocket_server(ws_server)

    loop = asyncio.get_running_loop()

    # --- Signal Handling ---
    # Set up signal handlers for graceful shutdown on SIGINT (Ctrl+C) and SIGTERM
    def signal_handler():
        logging.info("Shutdown signal received, initiating graceful shutdown...")
        shutdown_requested.set()

    # Attempt to add signal handlers
    try:
        for sig in (signal.SIGINT, signal.SIGTERM):
            loop.add_signal_handler(sig, signal_handler)
    except NotImplementedError:
        logging.warning(
            "Signal handlers not supported on this platform. Use Ctrl+C to stop."
        )

    # --- Start Servers ---
    try:
        # Start WebSocket Server
        ws_server_instance = await ws_server.start()
        if not ws_server_instance:
            logging.error("Failed to start WebSocket server. Exiting.")
            return  # Exit if WebSocket server fails to start

        # Start UDP Server (doesn't block now)
        await udp_handler.start_server(host=host, port=udp_port)

        # Keep main running until shutdown is requested
        logging.info("Servers started. Press Ctrl+C to stop.")
        await shutdown_requested.wait()  # Wait for the shutdown signal

    except OSError as e:
        logging.error(
            f"Server startup failed: {e}. Check if ports {ws_port} or {udp_port} are already in use."
        )
    except Exception as e:
        logging.error(f"An unexpected error occurred during server runtime: {e}")
    finally:
        logging.info("Shutting down servers...")
        # --- Graceful Shutdown ---
        # Stop WebSocket Server
        await ws_server.stop()

        # Stop UDP Server
        udp_handler.stop_server()

        # Clean up signal handlers if they were added
        if hasattr(loop, "remove_signal_handler"):
            for sig in (signal.SIGINT, signal.SIGTERM):
                try:
                    loop.remove_signal_handler(sig)
                except Exception as e:
                    logging.error(f"Error removing signal handler for {sig.name}: {e}")

        logging.info("Servers shut down gracefully.")


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logging.info("Server stopped with keyboard interrupt (CTRL+C)")
    except Exception as e:
        logging.error(f"Unexpected error: {e}", exception=e, exc_info=True)
