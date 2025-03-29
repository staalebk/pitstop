#ifndef ESPNOW_MANAGER_H
#define ESPNOW_MANAGER_H

#include <esp_now.h>
#include <WiFi.h>
#include "protocol.h"
extern bool isWiFiConnected();
uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void onESPSend(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS)
        return;
    Serial.print("[ESPNOW] Send Status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void onESPReceive(const esp_now_recv_info_t *recvInfo, const uint8_t *incomingData, int len) {
    if (len == sizeof(CarData)) {
        CarData *cd = (CarData *) incomingData;
        if (cd->magic != 0x13371337){
            Serial.println("Invalid ESPNOW magic");
            return;
        }
        memcpy(&carData, incomingData, sizeof(carData));
        Serial.print(".");
    } else {
        Serial.println("Invalid ESPNOW length");
    }
}

void setupESPNOW() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESPNOW] Init Failed");
    return;
  }

  esp_now_register_send_cb(onESPSend);
  esp_now_register_recv_cb(onESPReceive);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddr, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(broadcastAddr)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("[ESPNOW] Failed to add peer");
    }
  }
}

void sendChannelESPNOW() {
    static uint8_t counter = 0;
    uint8_t payload[5];
    uint32_t magic = 0x1337BEEF;
    memcpy(payload, &magic, 4);             // First 4 bytes = magic
    payload[4] = WiFi.channel();            // 5th byte = channel
    if (!counter++)
        Serial.printf("Sending ESPNOW channel %d\n", payload[4]);
  
    esp_err_t result = esp_now_send(broadcastAddr, payload, sizeof(payload));
    if (result != ESP_OK) {
      Serial.println("[ESPNOW] Send failed");
    }
  }

  TickType_t espWakeTime = xTaskGetTickCount();
  void espnowSendingThread(void* pvParams) {
    while (true) {
        if (isWiFiConnected()) {
            sendChannelESPNOW();
        }
        Serial.println("");
        vTaskDelayUntil(&espWakeTime, pdMS_TO_TICKS(1000));
    }
}
#endif
