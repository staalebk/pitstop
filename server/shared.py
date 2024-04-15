import time
# Contains the last 5 seconds of data for each sender, in 100 ms increments to be
# used as a buffer for sending to websocket clients

start = int(round(time.time() * 1000))
start = start // 1000 * 1000

active_senders = {
    "00000000-0000-0000-0000-000000000000": [
        {
            'id': "00000000-0000-0000-0000-000000000000",
            'timestamp': 100,
            'speed': 50,
            'heading': 90,
            'brake_temp_left': [50, 100, 150, 200, 250, 300, 350, 400, 450, 600, 800, 900, 1200, 600, 200, -5],
            'brake_temp_right': [500, 700, 700, 800, 900, 850, 600, 500, 450, 650, 700, 650, 400, 1300, 1300, 250],
            'position': (59.367778, 11.261944)
        },
        {
            'id': "00000000-0000-0000-0000-000000000000",
            'timestamp': 200,
            'speed': None,
            'heading': None,
            'brake_temp_left': None,
            'brake_temp_right': None,
            'position': (59.367778, 11.261944)
        },
        {
            'id': "00000000-0000-0000-0000-000000000000",
            'timestamp': 300,
            'speed': 100,
            'heading': 90,
            'brake_temp_left': [900, 0, 900, 0, 900, 0, 900, 0, 900, 0, 900, 0, 900, 0, 900, 450],
            'brake_temp_right': [500, 700, 700, 900, 1200, 850, 600, 500, 450, 650, 700, 650, 400, 300, 300, 250],
            'position': (59.367778, 11.261944)
        }
    ]
}

