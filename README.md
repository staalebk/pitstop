# pitstop
Information screen for all the people in the pits, and at home!

This consists of four parts:
1) ESP32 microcontroller transmitting data from the cars using UDP.
2) A Python script receives UDP packets, filters the duplicate data, and fixes out-of-order packets. It then inserts the serialized data into the database and provides live data to clients using Websocket.
3) A Postgresql database that stores the historical data.
4) The web frontend that shows the live data it gets from the websocket, and can also fetch historical stuff from the database using HTTP.


UDP Protocol:

The protocol contains two types of packets, authentication packets and data packets.

* Authentication packets
  * One byte to signify it is an authentication packet: 0xFF
  * Contains the 128 bit unique id (UUID) and nothing else
  * is transmitted every 10 seconds
  * Remote IP/Port is remembered and all packets from that IP/Port gets assigned that user.

* Data packets
  * One byte to tell which protocol version is in use: 0x00 for v0
  * 8 bytes (int64), microseconds since an arbitrary point in time for this packet.
  * Current data for vehicle:
    * 2 bytes, uint16, GPS speed in km/h * 100
    * 2 bytes, uint16, heading in degrees * 100
    * 2 bytes * 16, contains brake temperature for 16 locations on brake disc, uint16, 0.1 C per bit, -100 C offset.
    * 1 byte, uint8 heart rate of driver.
    * 1 byte, uint8 coolant temperature in celsius.
    * 1 byte, uint8 engine oil temperature in celsius.
    * 1 byte, uint8 accelerator %.
    * 1 byte, uint8 clutch %.
    * 1 byte, uint8 brake %.
    * 4 bytes, uint32_t, LAT * 6000000.0
    * 4 bytes, uint32_t, LONG * 6000000.0
    * 2 bytes, uint16, RPM
    * 2 bytes, int16, G Force X
    * 2 bytes, int16, G Force Y
    * 2 bytes, int16, G Force Z
    * 4 bytes, int32, Altitude (in mm)
    * 2 bytes, uint16, FL Wheel speed in km/h * 100
    * 2 bytes, uint16, FR Wheel speed in km/h * 100
    * 2 bytes, uint16, RL Wheel speed in km/h * 100
    * 2 bytes, uint16, RR Wheel speed in km/h * 100
