#include <Arduino.h>
#include <Preferences.h>
#include <WiFiUdp.h>
#include <Ticker.h>

#include "pitstop.h"

Ticker timer1;
Ticker timer2;
Preferences preferences;
const char *nvsNamespace = "storage";
const char *uuidKey = "deviceUUID";
const size_t UUID_SIZE = 16; // 128 bits represented in 16 bytes
AuthenticationPacket authpacket;
const char *udpEndpointIP;
uint16_t udpLocalPort;
uint16_t udpRemotePort;

WiFiUDP udp;

DataPacket current;
DataPacket datapacket;


void setLat(uint32_t ilat) {
    current.positions->latitude = ilat;
}

void setLon(uint32_t ilon) {
    current.positions->longitude = ilon;
}

void setRPM(uint32_t rpm) {
    if(rpm == 0xFFFF)
        rpm = 0;
    current.positions->rpm = rpm;
}

void setSpeed(uint32_t speed) {
    current.vehicleData.speed = speed;
}

void setHeading(uint32_t heading) {
    current.vehicleData.heading = heading;
}

int64_t getCurrentTimeMicros() {
  struct timeval tv; // Structure to hold the time in seconds and microseconds since the Epoch
  gettimeofday(&tv, NULL);
  int64_t microsSinceEpoch = ((int64_t)tv.tv_sec) * 1000000L + tv.tv_usec;
  return microsSinceEpoch;
}

void ensureUUID() {
    authpacket.packetType = 0xFF;
    uint8_t uuid[UUID_SIZE];
    preferences.begin(nvsNamespace, false); // Open NVS in read/write mode

    size_t readBytes = preferences.getBytes(uuidKey, uuid, UUID_SIZE);
    if (readBytes != UUID_SIZE) { // UUID not found or length mismatch
        Serial.println("No valid UUID found, generating new UUID...");

        // Use the ESP32 hardware random number generator to fill the UUID array
        for (int i = 0; i < UUID_SIZE; i++) {
            uuid[i] = esp_random() % 256; // esp_random returns a 32-bit number
        }

        // Save the newly generated UUID to NVS
        preferences.putBytes(uuidKey, uuid, UUID_SIZE);
        Serial.print("New UUID stored: ");
    } else {
        Serial.print("UUID found: ");
    }

    // Print the UUID in a readable format
    for (int i = 0; i < UUID_SIZE; i++) {
        Serial.printf("%02X", uuid[i]);
        if (i < UUID_SIZE - 1) Serial.print("-");
        authpacket.uuid[i] = uuid[i];
    }
    Serial.println();

    preferences.end(); // Close the NVS
}

void setupPitstop(const char *address, uint16_t remotePort, uint16_t localPort, uint8_t *uuid)
{
    ensureUUID();
    udp.begin(localPort);
    udpEndpointIP = address; 
    udpRemotePort = remotePort;
    udpLocalPort = localPort;
    memcpy(uuid, authpacket.uuid,16);
    timer1.attach(0.1, sendData);  // 0.1 seconds is 100 milliseconds
    timer2.attach(10, sendAuth);
}

void sendData(){
    datapacket.protocolVersion = 0x00;
    datapacket.timestamp = (getCurrentTimeMicros() / 100000) * 100000;
    datapacket.vehicleData.speed = current.vehicleData.speed;
    datapacket.vehicleData.heading = current.vehicleData.heading;
    for(int i = POS_PER_PACKET-1; i>0; i--) {
        datapacket.positions[i].rpm = datapacket.positions[i-1].rpm;
        datapacket.positions[i].latitude = datapacket.positions[i-1].latitude;
        datapacket.positions[i].longitude = datapacket.positions[i-1].longitude;
    }
    datapacket.positions[0].rpm = current.positions->rpm;
    datapacket.positions[0].latitude = current.positions->latitude;
    datapacket.positions[0].longitude = current.positions->longitude;
    udp.beginPacket(udpEndpointIP, udpRemotePort);
    udp.write((uint8_t *) &datapacket, sizeof(DataPacket));
    udp.endPacket();
}

void sendAuth(){
    udp.beginPacket(udpEndpointIP, udpRemotePort);
    udp.write((uint8_t *) &authpacket, sizeof(AuthenticationPacket));
    udp.endPacket();
}