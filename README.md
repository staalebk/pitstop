# pitstop
Information screen for all the people in the pits, and at home!

This consists of four parts:
1) ESP32 microcontroller transmitting data from the cars using UDP.
2) A Python script receives UDP packets, filters the duplicate data, and fixes out-of-order packets. It then inserts the serialized data into the database and provides live data to clients using Websocket.
3) A Postgresql database that stores the historical data.
4) The web frontend that shows the live data it gets from the websocket, and can also fetch historical stuff from the database using HTTP.
