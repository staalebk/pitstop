#pragma once
#include <WiFi.h>
#include "protocol.h"
#include "racebox.h"

const char* WIFI_SSID = "ez";  
const char* WIFI_PASS = "ez123456";  
const uint16_t PORT  = 1337;
IPAddress    broadcastIP   = IPAddress(255,255,255,255);
WiFiUDP broadcastudp;
bool wifiConnected;

const int PITSTOP_PORT = 5005;
const int UDP_LOCAL_PORT = 4210;
const char* PITSTOP_TARGET_IP = "172.232.157.107";
WiFiUDP pitstopudp;

extern RaceboxDataMessage latestGPS;
extern CurrentVehicleData latestVehicle;

void generateUUID(uint8_t uuidIn[16]) {
    static bool once = false;
    static uint8_t uuid[16];

    if (!once) {
	    uint8_t mac[6];
	    WiFi.macAddress(mac);      // use Arduino WiFi API
	    memcpy(uuid, mac, 6);

	    uint32_t chipId = (uint32_t)ESP.getEfuseMac(); // lower 32 bits of unique ID
	    memcpy(uuid + 6, &chipId, 4);

	    const uint8_t salt[6] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
	    memcpy(uuid + 10, salt, 6);
        Serial.print("UUID: ");
        for (int i = 0; i < 16; ++i) {
            if (i > 0) Serial.print(":");
            if (uuid[i] < 0x10) Serial.print("0"); // leading zero
            Serial.print(uuid[i], HEX);
        }
        Serial.println();

        once = true;
    }
    memcpy(uuidIn, uuid, 16);
}

void sendAuthUDP() {
	AuthenticationPacket packet = {};

	// Fill protocol version and timestamp
	packet.packetType = 0xFF;
    generateUUID(packet.uuid);

	// Send the packet
    pitstopudp.beginPacket(PITSTOP_TARGET_IP, PITSTOP_PORT);
	pitstopudp.write((uint8_t*)&packet, sizeof(AuthenticationPacket));
	pitstopudp.endPacket();
}


void sendGPSUDP(RaceboxDataMessage *data) {
    broadcastudp.beginPacket(broadcastIP, PORT);
    GPSData gpsdata = {};
    memcpy(&gpsdata.RDM, data, sizeof(RaceboxDataMessage));
    broadcastudp.write((uint8_t *)&gpsdata, sizeof(gpsdata));
    broadcastudp.endPacket();
}

void sendCarDataUDP() {
    pitstopudp.beginPacket(PITSTOP_TARGET_IP, PITSTOP_PORT);
    DataPacket datapacket = {};
    datapacket.timestamp = (int64_t)micros();
    // Update latestVehicle with data from Racebox
    latestVehicle.speed = (uint16_t) (latestGPS.speed * 0.36);
    latestVehicle.heading = (uint16_t) (latestGPS.heading / 10e2);
    latestVehicle.latitude = latestGPS.latitude*1.0/1e7 * 6000000.0f;
    latestVehicle.longitude = latestGPS.longitude*1.0/1e7 * 6000000.0f;
    latestVehicle.gForceX = latestGPS.gForceX;
    latestVehicle.gForceY = latestGPS.gForceY;
    latestVehicle.gForceZ = latestGPS.gForceZ;
    latestVehicle.altitude = latestGPS.mslAltitude;
    memcpy(&datapacket.vehicleData, &latestVehicle, sizeof(CurrentVehicleData));
    pitstopudp.write((uint8_t *)&datapacket, sizeof(datapacket));
    pitstopudp.endPacket();
}



TickType_t udpAuthWakeTime = xTaskGetTickCount();
void udpAuthSendingThread(void* pvParams) {
    while (true) {
        if (wifiConnected) {
            sendAuthUDP();
        }
        vTaskDelayUntil(&udpAuthWakeTime, pdMS_TO_TICKS(10000));
    }
}

TickType_t udpWakeTime = xTaskGetTickCount();
void udpSendingThread(void* pvParams) {
    while (true) {
        if (wifiConnected) {
            sendCarDataUDP();
        }
        vTaskDelayUntil(&udpWakeTime, pdMS_TO_TICKS(50));
    }
}