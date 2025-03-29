#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include "protocol.h"

const char* WIFI_SSID = "ez";
const char* WIFI_PASS = "ez123456";

const char* UDP_TARGET_IP = "84.211.23.46";
const int UDP_TARGET_PORT = 5005;
const int UDP_LOCAL_PORT = 4210;

WiFiUDP udp;
bool wifiConnected = false;
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
	} else {
		Serial.println("\n[WiFi] Connection FAILED");
		wifiConnected = false;
	}
}


void setupWiFi() {
  WiFi.mode(WIFI_STA);
  connectWiFi();
}

int wifiFailCount = 0;
const int maxFailsBeforeReboot = 5;

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
					Serial.println("[WiFi] Too many failures. Restarting ESP...");
					delay(1000);
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

TickType_t udpWakeTime = xTaskGetTickCount();
void udpSendingThread(void* pvParams) {
    while (true) {
        sendStatusUDP();
        Serial.printf("U");
        vTaskDelayUntil(&udpWakeTime, pdMS_TO_TICKS(100));
    }
}

TickType_t udpAuthWakeTime = xTaskGetTickCount();
void udpAuthSendingThread(void* pvParams) {
    while (true) {
        if (isWiFiConnected()) {
            sendAuthUDP();
        }
        Serial.printf("A");
        vTaskDelayUntil(&udpAuthWakeTime, pdMS_TO_TICKS(100));
    }
}



#endif
