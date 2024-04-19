#include<Arduino.h>
#include<BLEDevice.h>
#include<BLEServer.h>
#include<BLEUtils.h>
#include<BLE2902.h>
#include "ble.h"


BLEService*        mainService;
BLEServer*         mainServer;
BLEAdvertising*    mainAdvertising;
BLECharacteristic* GATTConfig;
BLECharacteristic* GATTMonitor;
bool deviceConnected = false;
bool deviceIndicating = false;
bool subscribeSuccess[MONITORS_MAX];
int32_t monitorValues[MONITORS_MAX];
CallbackType callbacks[MONITORS_MAX] = {nullptr}; // Array of function pointers

void registerCallback(int id, CallbackType callback) {
    if (id >= 0 && id < MONITORS_MAX) {
        callbacks[id] = callback;
    } else {
        Serial.println("Error: ID is out of range.");
    }
}

bool executeCallback(int id, uint32_t value) {
    if (id >= 0 && id < MONITORS_MAX && callbacks[id] != nullptr) {
        callbacks[id](value);  // Execute the callback function
        return true;
    }
    return false;
}


int32_t getData(int32_t data) {
    return monitorValues[data];
}

// Function to convert last two bytes of UUID to a hex string
String uuidToHexString(const uint8_t* uuid, int length) {
    String hexStr;
    for (int i = length - 2; i < length; i++) {  // Get last two bytes
        if (uuid[i] < 16) {
            hexStr += "0";  // Leading zero for single hex digit
        }
        hexStr += String(uuid[i], HEX);
    }
    hexStr.toUpperCase();  // Convert to upper case if needed
    return hexStr;
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        Serial.println("onConnect!");
        deviceConnected = true;
        for (int i = 0; i < MONITORS_MAX; ++i) {
           subscribeSuccess[i] = false;
    }
    };

    void onDisconnect(BLEServer* pServer) {
        Serial.println("onDisconnect!");
        deviceConnected = false;
        BLEDevice::startAdvertising();
    }
};

class MyCallbacks2: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0 && (value.length()%5 == 0)) {
            int values = value.length()/5;
            for(int i = 0; i < values; i++) {
                uint8_t mon = value[i*5];
                int32_t v = 0;
                v |= value[i*5+1] << 24;
                v |= value[i*5+2] << 16;
                v |= value[i*5+3] << 8;
                v |= value[i*5+4] << 0;
                monitorValues[mon] = v;

                executeCallback(mon, v);
            }
        /*
        Serial.print("DATA: ");
        for (int i = 0; i < value.length(); i++) {
          Serial.printf("%02X", value[i]);
          if(i == 0)
            Serial.print(" ");
        }
        Serial.println();
        */
        }
    }

    void onNotify(BLECharacteristic* pCharacteristic) {
        Serial.println("2RC is notifying?");
    }

    void onStatus(BLECharacteristic* pCharacteristic, Status s, uint32_t code) {
        //Serial.println("2RC status?");
        //Serial.println(s);
        //Serial.println(code);
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t* param) {
      Serial.println("onWrite!");
      std::string value = pCharacteristic->getValue();

      if (value.length() > 1) {
        if(value[0] == CMD_RESULT_OK) {
            Serial.print("Subscription ");
            Serial.printf("%02X", value[1]);
            Serial.println(" Successful");
            subscribeSuccess[value[1]] = true;
        }
        if(value[0] == CMD_RESULT_PAYLOAD_OUT_OF_SEQUENCE) {
            Serial.print("Subscription ");
            Serial.printf("%02X", value[1]);
            Serial.println(" Out of sequence");
        }
        if(value[0] == CMD_RESULT_EQUATION_EXCEPTION) {
            Serial.print("Subscription ");
            Serial.printf("%02X", value[1]);
            Serial.println(" Equation error");
        }
      } else if (value.length() > 0) {
        Serial.println("*********");
        Serial.print("New value: ");
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);
        Serial.println();
        // print in hex
        Serial.print("In hex: ");
        for (int i = 0; i < value.length(); i++)
          Serial.printf("%02X", value[i]);
        Serial.println();
        Serial.println("*********");
      }
    }

    void onNotify(BLECharacteristic* pCharacteristic) {
        //Serial.println("RC is notifying?");
    }

    void onStatus(BLECharacteristic* pCharacteristic, Status s, uint32_t code) {
        //Serial.println("RC status?");
        //Serial.println(s);
        //Serial.println(code);
    }
};

class DescrCallBack : public BLEDescriptorCallbacks
{
public:
  void onWrite(BLEDescriptor *desc) override
  {
    Serial.println("Data received!");
    deviceIndicating = true;
  }
};

void setupRCBle(uint8_t *UUID) {
    String hexPart = uuidToHexString(UUID, 16);
    String bleName = "RC DIY #" + hexPart;
    const char* RCBleName = bleName.c_str();
    setupRCBle(RCBleName);
}

void setupRCBle(const char *name) {
    uint8_t macaddr[6];
    BLEDevice::init(name);
    mainServer = BLEDevice::createServer();
    mainServer->setCallbacks(new MyServerCallbacks());
    mainService = mainServer->createService(BLEUUID((uint16_t)0x1FF8));
    GATTConfig = mainService->createCharacteristic(BLEUUID((uint16_t)0x0005), BLECharacteristic::PROPERTY_INDICATE | BLECharacteristic::PROPERTY_WRITE );
    GATTMonitor = mainService->createCharacteristic(BLEUUID((uint16_t)0x0006), BLECharacteristic::PROPERTY_WRITE_NR );
    //GATTthr = mainService->createCharacteristic(BLEUUID((uint16_t)0x0003), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY );
    BLE2902 *desc = new BLE2902();
    desc->setIndications(true);
    desc->setCallbacks(new DescrCallBack());
    GATTConfig->addDescriptor(desc);
    GATTConfig->setIndicateProperty(true);
    GATTConfig->setCallbacks(new MyCallbacks());
    BLE2902 *desc2 = new BLE2902();
    desc2->setCallbacks(new DescrCallBack());
    GATTMonitor->addDescriptor(desc2);
    GATTMonitor->setCallbacks(new MyCallbacks2());
    //GATTthr->addDescriptor(new BLE2902());
    mainService->start();
    mainAdvertising = BLEDevice::getAdvertising();
    mainAdvertising->addServiceUUID(BLEUUID((uint16_t)0x1FF8));
    mainAdvertising->setScanResponse(false);
    mainAdvertising->setMinPreferred(0x0);
    BLEDevice::startAdvertising();
    Serial.println("Waiting for RaceChrono connection...");
}

boolean sendConfigCommand(int cmdType, int monitorId, const char* payload, int payloadSequence = 0) {
    // Initially use CMD_TYPE_ADD instead of CMD_TYPE_ADD_INCOMPLETE
    cmdType = cmdType == CMD_TYPE_ADD_INCOMPLETE ? CMD_TYPE_ADD : cmdType;

    // Figure out payload
    char* remainingPayload = NULL; 
    char payloadPart[MAX_PAYLOAD_PART + 1];
    if (payload && cmdType == CMD_TYPE_ADD) {
        // Copy first 17 characters to payload
        strncpy(payloadPart, payload, MAX_PAYLOAD_PART);
        payloadPart[MAX_PAYLOAD_PART] = '\0';
        
         // If it does not fit to one payload, save the remaining part
        int payloadLen = strlen(payload);
        if (payloadLen > MAX_PAYLOAD_PART) {
            int remainingPayloadLen = payloadLen - MAX_PAYLOAD_PART;
            remainingPayload = (char*)malloc(remainingPayloadLen + 1);
            strncpy(remainingPayload, payload + MAX_PAYLOAD_PART, remainingPayloadLen);
            remainingPayload[remainingPayloadLen] = '\0';
            cmdType = CMD_TYPE_ADD_INCOMPLETE;
        }
    } else {
        payloadPart[0] = '\0';
    }
  
    // Indicate the characteristic
    byte bytes[20];
    bytes[0] = (byte)cmdType;
    bytes[1] = (byte)monitorId;
    bytes[2] = (byte)payloadSequence;
    memcpy(bytes + 3, payloadPart, strlen(payloadPart));
    GATTConfig->setValue(bytes,  3 + strlen(payloadPart));
    GATTConfig->indicate();

    // Handle remaining payload
    if (remainingPayload) {
        boolean r = sendConfigCommand(CMD_TYPE_ADD, monitorId, remainingPayload, payloadSequence + 1);
        free(remainingPayload);
        return r;
    } else {
        return true;
    }
}

void pollRCBle() {
    if(deviceConnected && deviceIndicating) {
        if(!subscribeSuccess[0])
            sendConfigCommand(CMD_TYPE_ADD, 0, "channel(device(gps), satellites)");
        if(!subscribeSuccess[1])
            sendConfigCommand(CMD_TYPE_ADD, 1, "channel(device(gps), latitude)");
        if(!subscribeSuccess[2])
            sendConfigCommand(CMD_TYPE_ADD, 2, "channel(device(gps), longitude)");
        if(!subscribeSuccess[3])
            sendConfigCommand(CMD_TYPE_ADD, 3, "channel(device(gps), speed)");
        if(!subscribeSuccess[4])
            sendConfigCommand(CMD_TYPE_ADD, 4, "channel(device(canbus), rpm)");
        if(!subscribeSuccess[5])
            sendConfigCommand(CMD_TYPE_ADD, 5, "channel(device(gps), bearing)");
        if(!subscribeSuccess[6])
            sendConfigCommand(CMD_TYPE_ADD, 6, "channel(device(gps), timestamp)");
        if(!subscribeSuccess[7])
            sendConfigCommand(CMD_TYPE_ADD, 7, "channel(device(hrm), heart_rate)");
        //deviceConfigured = true;
    }
}