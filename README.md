# pitstop
Information screen for all the people in the pits, and at home!

This consists of four parts:
1) ESP32 microcontroller transmitting data from the cars using UDP.
2) A Python script receives UDP packets, filters the duplicate data, and fixes out-of-order packets. It then inserts the serialized data into the database and provides live data to clients using Websocket.
3) A Postgresql database that stores the historical data.
4) The web frontend that shows the live data it gets from the websocket, and can also fetch historical stuff from the database using HTTP.


UDP Protocol:
* Authentication packets
  * Contains the 128 bit unique id (UUID) and nothing else
  * is transmitted every 10 seconds
  * Remote IP/Port is remembered and all packets from that IP/Port gets assigned that user.
* Data packets
  * 8 bytes (int64), milliseconds since 1970 for this packet
  * Current data:
    * 2 bytes speed (uint16) in km/h * 100
    * 2 bytes heading (uint16) degrees * 100
    * 16 * 2 bytes brake temperature in 16 spots, uint16, 0.1 C per bit, -100 C offset.
  * Array of the position of the last 40 * 100ms: (First elements are the current data)
    * 4 bytes single precision float LAT
    * 4 bytes single precision float LONG
    * 2 bytes RPM (uint16)
