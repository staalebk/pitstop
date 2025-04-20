#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include "protocol.h"

const char* WIFI_SSID = "ez";
const char* WIFI_PASS = "ez123456";
WiFiUDP udpBroadcast;
const uint16_t BROADCAST_PORT = 1337;   // same port your sender uses

//const char* UDP_TARGET_IP = "84.211.23.46";
const char* UDP_TARGET_IP = "172.232.157.107";
const int UDP_TARGET_PORT = 5005;
const int UDP_LOCAL_PORT = 4210;

WiFiUDP udp;
bool wifiConnected = false;
int staChannel;
unsigned long lastAttempt = 0;
const unsigned long reconnectInterval = 1000;

void connectWiFi() {
	WiFi.disconnect(true, true); // full disconnect and erase config
	delay(100);

	WiFi.begin(WIFI_SSID, WIFI_PASS);
	Serial.println("[WiFi] Connecting...");

	unsigned long start = millis();
	while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
		delay(500);
		Serial.print(".");
	}

	if (WiFi.status() == WL_CONNECTED) {
		Serial.println("\n[WiFi] Connected. IP: " + WiFi.localIP().toString());
		udp.begin(UDP_LOCAL_PORT);
		wifiConnected = true;
		if (!udpBroadcast.begin(BROADCAST_PORT)) {
			Serial.println("Failed to start BROADCAST UDP");
		} else {
		  Serial.printf("Listening for broadcast on port %u\n", BROADCAST_PORT);
		}
	} else {
		Serial.println("\n[WiFi] Connection FAILED");
		wifiConnected = false;
	}
}


void setupWiFi() {
  WiFi.mode(WIFI_AP_STA);
  connectWiFi();
}

int wifiFailCount = 0;
const int maxFailsBeforeReboot = 3;

void maintainWiFi() {
	if (millis() - lastAttempt > reconnectInterval) {
		lastAttempt = millis();

		if (WiFi.status() != WL_CONNECTED) {
			Serial.println("[WiFi] Disconnected. Reconnecting...");
			connectWiFi();

			if (WiFi.status() != WL_CONNECTED) {
				wifiFailCount++;
				Serial.printf("[WiFi] Fail count: %d\n", wifiFailCount);

				if (wifiFailCount >= maxFailsBeforeReboot) {
					Serial.println("[WiFi] Too many failures. Restarting ESP in 10 sec...");
                    WiFi.disconnect(true, true);
					delay(10000);
					ESP.restart();
				}
			}
		} else {
			wifiConnected = true;
			wifiFailCount = 0; // Reset fail count on success
		}
	}
}


bool isWiFiConnected() {
  return wifiConnected;
}


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
	udp.beginPacket(UDP_TARGET_IP, UDP_TARGET_PORT);
	udp.write((uint8_t*)&packet, sizeof(AuthenticationPacket));
	udp.endPacket();
}

void sendStatusUDP() {
	DataPacket packet = {};
    /*
	// Fill protocol version and timestamp
	packet.protocolVersion = 0x00;
	packet.timestamp = (int64_t)(micros()) / 100000 * 100000; // align to 100,000 µs

	// Mock data for CurrentVehicleData (replace with real values)
	packet.vehicleData.speed = carData.state.speed*100;                  // 123.45 km/h
	packet.vehicleData.heading = 9025;                 // 90.25°
	for (int i = 0; i < 16; ++i) {
		float tempC = 150.3;                            // example: 150.3°C
		packet.vehicleData.brakeTemp[i] = (uint16_t)((tempC + 100.0f) * 10.0f);
	}
	packet.vehicleData.heart_rate = 86;
	packet.vehicleData.coolant_temp = carData.state.waterTemp;
	packet.vehicleData.oil_temp = carData.state.oilTemp;
	packet.vehicleData.accelerator = carData.state.accel;
	packet.vehicleData.clutch = carData.state.clutch;
	packet.vehicleData.brake = carData.state.brake;

	// Mock positional data (replace with sensor input)
	for (int i = 0; i < POS_PER_PACKET; ++i) {
		packet.positions[i].latitude = (uint32_t)(59.123456 * 6000000.0);   // example: Oslo
		packet.positions[i].longitude = (uint32_t)(10.123456 * 6000000.0);
		packet.positions[i].rpm = 3450;
	}
    */
   if (xSemaphoreTake(packetMutex, portMAX_DELAY)) {
        udp.beginPacket(UDP_TARGET_IP, UDP_TARGET_PORT);
        udp.write((uint8_t*)&simulatedPacket, sizeof(DataPacket));
        udp.endPacket();
        
        //Serial.printf("Heading %u, RPM: %u\n", simulatedPacket.vehicleData.heading, simulatedPacket.positions[0].rpm);
        xSemaphoreGive(packetMutex);
    }
    /*
	// Send the packet
	udp.beginPacket(UDP_TARGET_IP, UDP_TARGET_PORT);
	udp.write((uint8_t*)&packet, sizeof(DataPacket));
	udp.endPacket();
    */
}

void readBroadcast() {
	int packetSize = udpBroadcast.parsePacket();
	if (packetSize <= 0) {
	  // no packet, nothing to do
	  return;
	}
	// make sure it’s the size we expect
	if (packetSize != sizeof(GPSData)) {
		Serial.printf("Unexpected packet size (want %u)\n", sizeof(RaceboxDataMessage));
		// you could still read and discard it:
		uint8_t discardBuf[packetSize];
		udpBroadcast.read(discardBuf, packetSize);
		return;
	}
	GPSData gps;
	int len = udpBroadcast.read(reinterpret_cast<uint8_t*>(&gps), sizeof(gps));
	if (len != sizeof(gps)) {
		Serial.printf("  → Read error: got %d bytes\n", len);
		return;
	}
	if (gps.magic != 0x1337D00F) {
		Serial.println("Invalid GPS UDP magic");
		return;            
	}
	memcpy(&gpsData, &gps, sizeof(gpsData));
	static uint8_t counter;
	if(!counter++)
		Serial.printf("fix: %d\n", gpsData.RDM.fixStatus);
	Serial.print("G");
}

TickType_t broadcastudpWakeTime = xTaskGetTickCount();
void broadcastudpreceivingThread(void* pvParams) {
    while (true) {
		readBroadcast();
        vTaskDelayUntil(&broadcastudpWakeTime, pdMS_TO_TICKS(40));
    }
}

TickType_t udpWakeTime = xTaskGetTickCount();
void udpSendingThread(void* pvParams) {
    while (true) {
        if (isWiFiConnected()) {
            sendStatusUDP();
            Serial.printf("U");
        }
        vTaskDelayUntil(&udpWakeTime, pdMS_TO_TICKS(50));
    }
}

TickType_t udpAuthWakeTime = xTaskGetTickCount();
void udpAuthSendingThread(void* pvParams) {
    while (true) {
        if (isWiFiConnected()) {
            sendAuthUDP();
            Serial.printf("A");
        }
        vTaskDelayUntil(&udpAuthWakeTime, pdMS_TO_TICKS(10000));
    }
}



#endif
