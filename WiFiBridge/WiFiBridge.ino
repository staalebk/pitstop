#include "protocol.h"
#include "FakeDataGenerator.h"
#include "screen.h"
#include "WiFiManager.h"
#include "ESPNOWManager.h"

CarData carData;

void setup() {
    setupScreen();
    drawSpeed(0);
    Serial.begin(115200);
    setupWiFi();
    setupESPNOW();
    packetMutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(simulateCarLoop, "FakeDataTask", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(udpSendingThread, "UDPSendingThread", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(udpAuthSendingThread, "UDPAuthSendingThread", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(espnowSendingThread, "espnowSendingThread", 4096, NULL, 1, NULL, 1);
}

void loop() {
    static uint8_t counter;
    maintainWiFi();

    drawSpeed(simulatedPacket.vehicleData.speed*1.0/100);
    //delay(100);
}