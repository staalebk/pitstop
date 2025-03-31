#pragma once



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

typedef struct __attribute__((packed)) {
    uint16_t packetStart;  // Always 0xB5 0x62
    uint8_t packetClass;
    uint8_t packetId;
    uint16_t payloadLength;
    RaceboxDataMessage RDM; //RaceboxDataMessage
} RaceboxPacket;

struct __attribute__((packed)) GPSData {
    uint64_t magic;   // Magic bytes
    RaceboxDataMessage RDM;   // Nested struct

    GPSData() : magic(0x1337D00F) {}
};
