#pragma once
#include <stdint.h>
#define POS_DIVIDER 6000000.0

#define POS_PER_PACKET 40
typedef struct __attribute__((packed)) {
    uint8_t packetType;  // Always 0xFF to signify an authentication packet
    uint8_t uuid[16];    // 128-bit unique ID (UUID)
} AuthenticationPacket;

typedef struct __attribute__((packed)) {
    uint16_t speed;                       // Speed in km/h * 100
    uint16_t heading;                     // Heading in degrees * 100
    uint8_t heart_rate;                  // heart_rate
    uint16_t brakeTemp[16];               // Brake temperatures for 16 locations
} CurrentVehicleData;

typedef struct __attribute__((packed)) {
    uint32_t latitude;                    // Latitude
    uint32_t longitude;                   // Longitude
    uint16_t rpm;                         // RPM
} PositionalData;

typedef struct __attribute__((packed)) {
    uint8_t protocolVersion;              // Protocol version, e.g., 0x00 for v0
    int64_t timestamp;                    // Microseconds since 1970, aligned to 100,000 microseconds
    CurrentVehicleData vehicleData;       // Current vehicle data
    PositionalData positions[POS_PER_PACKET];         // Array of positional data, 40 entries
} DataPacket;


// callbacks
void setLat(uint32_t);
void setLon(uint32_t);
void setRPM(uint32_t);
void setSpeed(uint32_t);
void setHeading(uint32_t);
void setHeartrate(uint32_t);

void setupPitstop(const char *address, uint16_t remotePort, uint16_t localPort, uint8_t *uuid);
void sendData();
void sendAuth();