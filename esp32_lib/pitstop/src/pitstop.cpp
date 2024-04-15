#include <Arduino.h>
#include <Preferences.h>
#include <WiFiUdp.h>
#include <Ticker.h>

#include "pitstop.h"

Ticker timer;
Preferences preferences;
const char *nvsNamespace = "storage";
const char *uuidKey = "deviceUUID";
const size_t UUID_SIZE = 16; // 128 bits represented in 16 bytes
AuthenticationPacket authpacket;
const char *udpEndpointIP;
uint16_t udpLocalPort;
uint16_t udpRemotePort;

WiFiUDP udp;

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

void setupPitstop(const char *address, uint16_t remotePort, uint16_t localPort)
{
    ensureUUID();
    udp.begin(localPort);
    udpEndpointIP = address; 
    udpRemotePort = remotePort;
    udpLocalPort = localPort;
    timer.attach(0.1, sendData);  // 0.1 seconds is 100 milliseconds
    //timer.attach(10, sendAuth);  // 0.1 seconds is 100 milliseconds
}

void sendData(){
    Serial.print("p");
    DataPacket dp;
    dp.protocolVersion = 0x00;
    dp.timestamp = (getCurrentTimeMicros() / 100000) * 100000;
    dp.vehicleData.speed = 100;
    dp.vehicleData.heading = 1337;
    for(int i = 0; i<POS_PER_PACKET; i++) {
        dp.positions[i].rpm = i*10;
        dp.positions[i].latitude = i*10;
        dp.positions[i].longitude = i*10;
    }
    udp.beginPacket(udpEndpointIP, udpRemotePort);
    udp.write((uint8_t *) &dp, sizeof(DataPacket));
    udp.endPacket();
    Serial.print("s");
}


void sendAuth(){
    udp.beginPacket(udpEndpointIP, udpRemotePort);
    udp.write((uint8_t *) &authpacket, sizeof(AuthenticationPacket));
    udp.endPacket();
    Serial.println("Authentication packet sent!");
}