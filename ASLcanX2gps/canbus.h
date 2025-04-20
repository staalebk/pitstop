#pragma once
#include <Arduino.h>
#include <driver/twai.h>
#include "e46.h"
#include "protocol.h"
#include "helper_functions.h"

bool isCanBusConnected = false;
extern CurrentVehicleData latestVehicle;
extern bool hasCAN;
void taskCanBusLoop(void *); // forward declaration
void parseCAN(twai_message_t *message); // ...

void canBusSetup() {
    // CAN1 setup.
    Serial.println("Initializing builtin CAN peripheral");
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN1_TX, (gpio_num_t)CAN1_RX, TWAI_MODE_LISTEN_ONLY /*TWAI_MODE_NORMAL*/);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = {
      .acceptance_code = ((0x0100 << 3) << 16) | (0x0400 << 3),
      .acceptance_mask = 0xF7FFDFFF,
      .single_filter = false,
    };
  
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
      Serial.println("CAN1 Driver initialized");
    } else {
      Serial.println("Failed to initialze CAN1 driver");
      return;
    }
  
    if (twai_start() == ESP_OK) {
      Serial.println("CAN1 interface started");
    } else {
      Serial.println("Failed to start CAN1");
      return;
    }
  
    // Disable CAN alerts, as we don't act on them anyway.
    // uint32_t alerts_to_enable = TWAI_ALERT_TX_IDLE | TWAI_ALERT_TX_SUCCESS | TWAI_ALERT_TX_FAILED | TWAI_ALERT_RX_QUEUE_FULL | TWAI_ALERT_RX_DATA | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR;
    // if (twai_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK) {
    //   Serial.println("CAN1 Alerts reconfigured");
    // } else {
    //   Serial.println("Failed to reconfigure alerts");
    //   return;
    // }
  
    isCanBusConnected = true;
    xTaskCreatePinnedToCore(taskCanBusLoop, "CAN bus reader", 16384, nullptr, 2, nullptr, 1);
}


static bool canPidAllowed(uint32_t pid) {
    switch (pid) {
      case can_asc1_id:
      case can_asc2_id:
      case can_asc3_id:
      //case can_asc4_id:
      //case can_lws1_id:
      case can_dme1_id:
      case can_dme2_id:
      //case can_dme3_id:
      case can_dme4_id:
      case can_icl2_id:
      case can_icl3_id:
        return true;
    }
  
    return false;
}


void canBusLoop() {
    // Manage CAN-Bus connection
    if (!isCanBusConnected) {
        // Connect to CAN-Bus
        Serial.println("Connecting CAN-Bus...");
        if (twai_start() == ESP_OK) {
            isCanBusConnected = true;
            Serial.println("CAN1 interface started");
        } else {
            Serial.println("Failed to start CAN1");
            delay(3000);
            return;
        }
    }

    // Handle CAN-Bus data
    if (!isCanBusConnected) {  // TODO: use driver status as flag
        vTaskDelay(pdMS_TO_TICKS(500));
        return;
    }

    #define CAN_POLLING_RATE_MS 1
    twai_message_t message;
    while (twai_receive(&message, pdMS_TO_TICKS(CAN_POLLING_RATE_MS)) == ESP_OK) {
        if (message.rtr) {
            // Not interested
            continue;
        }
        if (!canPidAllowed(message.identifier)) {
            static uint8_t counter = 0;
            if (counter == 255)
                Serial.printf("Ignoring pid: %d\n", message.identifier);
            continue;
        }
        parseCAN(&message);
    }
}

// **Parse CAN message and update receivedData struct**
void parseCAN(twai_message_t *message) {
    hasCAN = true;
    if (message->identifier == can_dme2_id) { // 0x329 / 809
        // **Accelerator percentage** (Byte 5, full byte)
        latestVehicle.accelerator = bytestouint(message->data, 5, 1) / 2.56;

        // **Brake percentage** (Bit 55)
        latestVehicle.brake = bitstouint(message->data, 48) * 100;

        // **Clutch percentage** (Bit 31)
        latestVehicle.clutch = bitstouint(message->data, 24) * 100;

        // **Coolant temperature** (Byte 1)
        latestVehicle.coolant_temp = (bytestouint(message->data, 1, 1) * 0.75) - 48;
    } 
    else if (message->identifier == 790) { // 0x2C5
        // **RPM Calculation** (Little-Endian, Bytes 2-3)
        latestVehicle.rpm = bytestouintle(message->data, 2, 2) * 0.15625;
    } 
    else if (message->identifier == 1349) {
        // **Oil temperature** (Byte 4)
        latestVehicle.oil_temp = bytestouint(message->data, 4, 1) - 48;
    }
    else if (message->identifier == 0x153 && false) {
        uint8_t vss_low_5 = (message->data[1] >> 3) & 0x1F;
        uint16_t vss_high_8 = message->data[2];
        uint16_t vss_raw = (vss_high_8 << 5) | vss_low_5;
        //carData.state.speed = (vss_raw - 0x160) / 16;
        static uint8_t counter = 0;
        if (!counter) {
          Serial.printf("low: %d high %d\n", vss_low_5, vss_high_8);
          Serial.printf("Speed v1 %f\n", carData.state.speed);
        }
        // ( (HEX[MSB] * 256 + HEX[LSB]) - 352 ) / 127
        //carData.state.speed = ((message->data[2] * 256 + message->data[1]) - 352) / 127;
        if (!counter)
          Serial.printf("Speed v2 %f\n", carData.state.speed);

        counter++;
          
    }
    else if (message->identifier == 0x613) {
      static uint8_t counter = 0;
      if (!counter++)
        hexDump(message->data, message->data_length_code);
      Serial.printf("byte 2: %d byte 5: %d\n", message->data[2], message->data[5]);
      latestVehicle.fuel_level = message->data[2] << 8 | message->data[5];
      //carData.state.fuelr = message->data[2];
      //carData.state.fuell = message->data[5];
    } else if (message->identifier == 0x1F0) {
        uint16_t raw1 = ((message->data[1] & 0x1F) << 8) | message->data[0];
        uint16_t raw2 = ((message->data[3] & 0x1F) << 8) | message->data[2];
        uint16_t raw3 = ((message->data[5] & 0x1F) << 8) | message->data[4];
        uint16_t raw4 = ((message->data[7] & 0x1F) << 8) | message->data[6];
        float speed1 = (float)(raw1-7)/15.875;
        float speed2 = (float)(raw2-7)/15.875;
        float speed3 = (float)(raw3-7)/15.875;
        float speed4 = (float)(raw4-7)/15.875;
        latestVehicle.wheel_speed_FL = speed1 * 100;
        latestVehicle.wheel_speed_FR = speed2 * 100;
        latestVehicle.wheel_speed_RL = speed3 * 100;
        latestVehicle.wheel_speed_RR = speed4 * 100;
    }
}



void taskCanBusLoop(void *) {
    for (;;) {
        canBusLoop();
      }
}