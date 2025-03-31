#pragma once
#include <stdint.h>


// UDP PROTOCOL
#define POS_PER_PACKET 40
typedef struct __attribute__((packed)) {
    uint8_t packetType;  // Always 0xFF to signify an authentication packet
    uint8_t uuid[16];    // 128-bit unique ID (UUID)
} AuthenticationPacket;

typedef struct __attribute__((packed)) {
	uint16_t speed;            // km/h * 100
	uint16_t heading;          // degrees * 100
	uint16_t brakeTemp[16];    // brake temps, 0.1°C per bit, -100°C offset
	uint8_t heart_rate;        // bpm
	uint8_t coolant_temp;      // °C
	uint8_t oil_temp;          // °C
	uint8_t accelerator;       // %
	uint8_t clutch;            // %
	uint8_t brake;             // %
} CurrentVehicleData;

typedef struct __attribute__((packed)) {
	uint32_t latitude;         // LAT * 6,000,000.0
	uint32_t longitude;        // LONG * 6,000,000.0
	uint16_t rpm;              // RPM
} PositionalData;

typedef struct __attribute__((packed)) {
	uint8_t protocolVersion;                // Protocol version (e.g. 0x00)
	int64_t timestamp;                      // Microseconds since epoch (aligned to 100,000 µs)
	CurrentVehicleData vehicleData;         // Current vehicle info
	PositionalData positions[POS_PER_PACKET]; // 40 entries, most recent first
} DataPacket;

// ESPNOW PROTOCOL

struct __attribute__((packed)) CarState {
    uint8_t oilTemp;  // Oil temperature (°C)       1 1
    uint8_t accel;    // Accelerator position (%)   1 2
    uint8_t brake;    // Brake position (%)         1 3
    uint8_t clutch;   // Clutch position (%)        1 4
    float speed;      // Speed (km/h or mph)        4 8
    uint16_t rpm;     // Engine RPM                 2 10
    uint8_t waterTemp;// Water temperature (°C)     1 11
    uint8_t fuelr;
    uint8_t fuell;

    CarState() : rpm(1337), oilTemp(66), waterTemp(69), speed(42.0), accel(1), brake(2), clutch(3), fuelr(13), fuell(13) {}
};

struct __attribute__((packed)) CarData {
    uint64_t magic;   // Magic bytes
    CarState state;   // Nested struct

    CarData() : magic(0x13371337) {}
};

typedef struct __attribute__((packed)) {
    uint32_t iTOW;                // 0
	uint16_t year;                // 4
	uint8_t  month;               // 6
	uint8_t  day;                 // 7
	uint8_t  hour;                // 8
	uint8_t  minute;              // 9
	uint8_t  second;              // 10
	uint8_t  validFlags;          // 11
	uint32_t timeAccuracy;        // 12
	int32_t  nanoseconds;         // 16
	uint8_t  fixStatus;           // 20
	uint8_t  fixStatusFlags;      // 21
	uint8_t  dateTimeFlags;       // 22
	uint8_t  numSVs;              // 23
	int32_t  longitude;           // 24
	int32_t  latitude;            // 28
	int32_t  wgsAltitude;         // 32
	int32_t  mslAltitude;         // 36
	uint32_t horizontalAccuracy;  // 40
	uint32_t verticalAccuracy;    // 44
	int32_t  speed;               // 48
	int32_t  heading;             // 52
	uint32_t speedAccuracy;       // 56
	uint32_t headingAccuracy;     // 60
	uint16_t pdop;                // 64
	uint8_t  latLonFlags;         // 66
	uint8_t  batteryStatus;       // 67
	int16_t  gForceX;             // 68
	int16_t  gForceY;             // 70
	int16_t  gForceZ;             // 72
	int16_t  rotRateX;            // 74
	int16_t  rotRateY;            // 76
	int16_t  rotRateZ;            // 78
} RaceboxDataMessage;

struct __attribute__((packed)) GPSData {
    uint64_t magic;   // Magic bytes
    RaceboxDataMessage RDM;   // Nested struct

    GPSData() : magic(0x1337D00F) {}
};

// Declare a global instance to be shared across threads
extern CarData carData;
extern GPSData gpsData;