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


void setLat(uint32_t ilat) {
    if(ilat == 0x7FFFFFFF)
        ilat = 0;
    current.positions->latitude = ilat/POS_DIVIDER;
}

void setLon(uint32_t ilon) {
    if(ilon == 0x7FFFFFFF)
        ilon = 0;
    current.positions->longitude = ilon/POS_DIVIDER;
}

void setRPM(uint32_t rpm) {
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
    timer1.attach(1, sendData);  // 0.1 seconds is 100 milliseconds
    timer2.attach(10, sendAuth);  // 0.1 seconds is 100 milliseconds
}

void sendData(){
    DataPacket dp;
    dp.protocolVersion = 0x00;
    dp.timestamp = (getCurrentTimeMicros() / 100000) * 100000;
    dp.vehicleData.speed = current.vehicleData.speed;
    dp.vehicleData.heading = current.vehicleData.heading;
    for(int i = 1; i<POS_PER_PACKET; i++) {
        dp.positions[i-1].rpm = dp.positions[i].rpm;
        dp.positions[i-1].latitude = dp.positions[i].latitude;
        dp.positions[i-1].longitude = dp.positions[i].longitude;
    }
    dp.positions->rpm = current.positions->rpm;
    dp.positions->latitude = current.positions->latitude;
    dp.positions->longitude = current.positions->longitude;
    udp.beginPacket(udpEndpointIP, udpRemotePort);
    udp.write((uint8_t *) &dp, sizeof(DataPacket));
    udp.endPacket();


void sendAuth(){
    udp.beginPacket(udpEndpointIP, udpRemotePort);
    udp.write((uint8_t *) &authpacket, sizeof(AuthenticationPacket));
    udp.endPacket();
}