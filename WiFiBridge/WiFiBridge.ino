#include "protocol.h"
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
}

void loop() {
    carData.state.speed += 0.1;
    maintainWiFi();

    if (isWiFiConnected()) {
        sendChannelESPNOW();
        sendAuthUDP();
    }
    drawSpeed(carData.state.speed);
    delay(1000);
}