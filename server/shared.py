import time
from collections import deque

# Contains the last 5 seconds of data for each sender, in 100 ms increments to be
# used as a buffer for sending to websocket clients

start = int(round(time.time() * 1000))
start = start // 1000 * 1000

active_senders = {
    # 'abc-123': deque([], maxlen=100)
}

