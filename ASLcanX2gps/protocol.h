#pragma once
#include <stdint.h>


// UDP PROTOCOL
#define POS_PER_PACKET 40
typedef struct __attribute__((packed)) {
    uint8_t packetType;  // Always 0xFF to signify an authentication packet
    uint8_t uuid[16];    // 128-bit unique ID (UUID)
} AuthenticationPacket;

typedef struct __attribute__((packed)) {
	uint16_t speed;            // GPS speed in km/h * 100
	uint16_t heading;          // degrees * 100
	uint16_t brakeTemp[16];    // brake temps, 0.1°C per bit, -100°C offset
	uint8_t heart_rate;        // bpm
	uint8_t coolant_temp;      // °C
	uint8_t oil_temp;          // °C
	uint8_t accelerator;       // %
	uint8_t clutch;            // %
	uint8_t brake;             // %
	uint32_t latitude;         // LAT * 6,000,000.0
	uint32_t longitude;        // LONG * 6,000,000.0
	uint16_t rpm;              // RPM
    int16_t gForceX;
    int16_t gForceY;
    int16_t gForceZ;
    int32_t altitude;
    uint16_t wheel_speed_FL;
    uint16_t wheel_speed_FR;
    uint16_t wheel_speed_RL;
    uint16_t wheel_speed_RR;
    uint16_t fuel_level;
} CurrentVehicleData;

typedef struct __attribute__((packed)) {
} PositionalData;

typedef struct __attribute__((packed)) DataPacket{
	uint8_t protocolVersion;                // Protocol version (e.g. 0x00)
	int64_t timestamp;                      // Microseconds since epoch (aligned to 100,000 µs)
	CurrentVehicleData vehicleData;         // Current vehicle info

    DataPacket () : protocolVersion(0x00) {}
};

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

// Declare a global instance to be shared across threads
extern CarData carData;