
/** NimBLE_Client Demo:
 *
 *  Demonstrates many of the available features of the NimBLE client library.
 *
 *  Created: on March 24 2020
 *      Author: H2zero
 */

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <esp_now.h>
#include <WiFi.h>
extern "C" {
    #include "esp_wifi.h"
  }
#include "racebox.h"

#define CHANNEL_SCAN_INTERVAL_MS 2000
#define ESPNOW_MAGIC 0x1337BEEF

uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint8_t current_channel = 1;
bool locked_channel = false;

static const NimBLEAdvertisedDevice* advDevice;
static bool                          doConnect  = false;
static uint32_t                      scanTimeMs = 5000; /** scan time in milliseconds, 0 = scan forever */

char model[100];
char serial[100];
char fw[100];
char hw[100];
char manufacturer[100];
RaceboxDataMessage latest;

void sendGPSESPNOW(RaceboxDataMessage *data) {
    static uint8_t counter = 0;
    GPSData gpsdata = {};
    memcpy(data, &gpsdata.RDM, sizeof(RaceboxDataMessage));
  
    esp_err_t result = esp_now_send(broadcastAddr, (uint8_t *)&gpsdata, sizeof(gpsdata));
    if (result != ESP_OK) {
      Serial.printf("[ESPNOW] Send failed %d", sizeof(gpsdata));
    }
}

 /**  None of these are required as they will be handled by the library with defaults. **
  **                       Remove as you see fit for your needs                        */
 class ClientCallbacks : public NimBLEClientCallbacks {
     void onConnect(NimBLEClient* pClient) override { Serial.printf("Connected\n"); }
 
     void onDisconnect(NimBLEClient* pClient, int reason) override {
         Serial.printf("%s Disconnected, reason = %d - Starting scan\n", pClient->getPeerAddress().toString().c_str(), reason);
         NimBLEDevice::getScan()->start(scanTimeMs, false, true);
     }
 
     /********************* Security handled here *********************/
     void onPassKeyEntry(NimBLEConnInfo& connInfo) override {
         Serial.printf("Server Passkey Entry\n");
         /**
          * This should prompt the user to enter the passkey displayed
          * on the peer device.
          */
         NimBLEDevice::injectPassKey(connInfo, 123456);
     }
 
     void onConfirmPasskey(NimBLEConnInfo& connInfo, uint32_t pass_key) override {
         Serial.printf("The passkey YES/NO number: %" PRIu32 "\n", pass_key);
         /** Inject false if passkeys don't match. */
         NimBLEDevice::injectConfirmPasskey(connInfo, true);
     }
 
     /** Pairing process complete, we can check the results in connInfo */
     void onAuthenticationComplete(NimBLEConnInfo& connInfo) override {
         if (!connInfo.isEncrypted()) {
             Serial.printf("Encrypt connection failed - disconnecting\n");
             /** Find the client with the connection handle provided in connInfo */
             NimBLEDevice::getClientByHandle(connInfo.getConnHandle())->disconnect();
             return;
         }
     }
 } clientCallbacks;
 
 /** Define a class to handle the callbacks when scan events are received */
 class ScanCallbacks : public NimBLEScanCallbacks {
     void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
         Serial.printf("Advertised Device found: %s\n", advertisedDevice->toString().c_str());
         if (advertisedDevice->getName() == "RaceBox Mini S 2231802051") {
         //if (advertisedDevice->isAdvertisingService(NimBLEUUID("DEAD"))) {
             Serial.printf("Found Our Service\n");
             /** stop scan before connecting */
             NimBLEDevice::getScan()->stop();
             /** Save the device reference in a global for the client to use*/
             advDevice = advertisedDevice;
             /** Ready to connect now */
             doConnect = true;
         }
     }
 
     /** Callback to process the results of the completed scan or restart it */
     void onScanEnd(const NimBLEScanResults& results, int reason) override {
         Serial.printf("Scan Ended, reason: %d, device count: %d; Restarting scan\n", reason, results.getCount());
         NimBLEDevice::getScan()->start(scanTimeMs, false, true);
     }
 } scanCallbacks;
 
 /** Notification / Indication receiving handler callback */
 void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    /*
    std::string str  = (isNotify == true) ? "Notification" : "Indication";
    str             += " from ";
    str             += pRemoteCharacteristic->getClient()->getPeerAddress().toString();
    str             += ": Service = " + pRemoteCharacteristic->getRemoteService()->getUUID().toString();
    str             += ", Characteristic = " + pRemoteCharacteristic->getUUID().toString();
    str             += ", Value = " + std::string((char*)pData, length);
    Serial.printf("%s\n", str.c_str());
    */
    byte CK_A = 0, CK_B = 0;
    for (int i = 2; i < length-2; i++) {
        CK_A = CK_A + pData[i];
        CK_B = CK_B + CK_A;
    }
    if (pData[length-2] != CK_A || pData[length-1] != CK_B) {
        Serial.printf("Invalid checksum!\n");
        return;
    }
    if (length < 8) {
        Serial.printf("Invalid length! %d\n", length);
        return;
    }
    RaceboxPacket *rp = (RaceboxPacket *)pData;
    if(rp->packetStart != 0x62B5) {
        Serial.printf("Invalid header! %d\n", rp->packetStart);
        return;
    }

    if(rp->packetClass != 255 || rp->packetId != 1 || rp->payloadLength != 80) {
        Serial.printf("Class: %d ID: %d Size: %d\n", rp->packetClass, rp->packetId, rp->payloadLength);
        return;
    }
    if (length < (rp->payloadLength + 8)) {
        Serial.printf("Not enoug bytes. Needs buffering?\n");
        return;
    }
    memcpy(&latest, &rp->RDM, sizeof(RaceboxDataMessage));
    sendGPSESPNOW(&rp->RDM);
    static uint8_t counter;
    counter ++;
    if (counter == 25){
        Serial.printf("-----\n");
        Serial.printf("Fix: %d (sats: %d) accuracy (hor: %d vert: %d) Battery %d%%\n", latest.fixStatus, latest.numSVs, latest.horizontalAccuracy, latest.verticalAccuracy, latest.batteryStatus & 0x7F);
        Serial.printf("Pos: %f %f\n", latest.latitude*1.0/1e7, latest.longitude*1.0/1e7);
        Serial.printf("%d-%02d-%02d %02d:%02d:%02d.%d\n", latest.year, latest.month, latest.day, latest.hour, latest.minute, latest.second, latest.nanoseconds);
        counter = 0;
    }
}
 
 /** Handles the provisioning of clients and connects / interfaces with the server */
 bool connectToServer() {
     NimBLEClient* pClient = nullptr;
 
     /** Check if we have a client we should reuse first **/
     if (NimBLEDevice::getCreatedClientCount()) {
         /**
          *  Special case when we already know this device, we send false as the
          *  second argument in connect() to prevent refreshing the service database.
          *  This saves considerable time and power.
          */
         pClient = NimBLEDevice::getClientByPeerAddress(advDevice->getAddress());
         if (pClient) {
             if (!pClient->connect(advDevice, false)) {
                 Serial.printf("Reconnect failed\n");
                 return false;
             }
             Serial.printf("Reconnected client\n");
         } else {
             /**
              *  We don't already have a client that knows this device,
              *  check for a client that is disconnected that we can use.
              */
             pClient = NimBLEDevice::getDisconnectedClient();
         }
     }
 
     /** No client to reuse? Create a new one. */
     if (!pClient) {
         if (NimBLEDevice::getCreatedClientCount() >= NIMBLE_MAX_CONNECTIONS) {
             Serial.printf("Max clients reached - no more connections available\n");
             return false;
         }
 
         pClient = NimBLEDevice::createClient();
 
         Serial.printf("New client created\n");
 
         pClient->setClientCallbacks(&clientCallbacks, false);
         /**
          *  Set initial connection parameters:
          *  These settings are safe for 3 clients to connect reliably, can go faster if you have less
          *  connections. Timeout should be a multiple of the interval, minimum is 100ms.
          *  Min interval: 12 * 1.25ms = 15, Max interval: 12 * 1.25ms = 15, 0 latency, 150 * 10ms = 1500ms timeout
          */
         pClient->setConnectionParams(6, 6, 0, 500);
 
         /** Set how long we are willing to wait for the connection to complete (milliseconds), default is 30000. */
         pClient->setConnectTimeout(5 * 1000);
 
         if (!pClient->connect(advDevice)) {
             /** Created a client but failed to connect, don't need to keep it as it has no data */
             NimBLEDevice::deleteClient(pClient);
             Serial.printf("Failed to connect, deleted client\n");
             return false;
         }
     }
 
     if (!pClient->isConnected()) {
         if (!pClient->connect(advDevice)) {
             Serial.printf("Failed to connect\n");
             return false;
         }
     }
 
     Serial.printf("Connected to: %s RSSI: %d\n", pClient->getPeerAddress().toString().c_str(), pClient->getRssi());
 
    /** Now we can read/write/subscribe the characteristics of the services we are interested in */
    NimBLERemoteService*        pSvc = nullptr;
    NimBLERemoteCharacteristic* pChr = nullptr;
    NimBLERemoteDescriptor*     pDsc = nullptr;
 
    // Read info
    pSvc = pClient->getService("0000180a-0000-1000-8000-00805f9b34fb");
    if (!pSvc)
        return false;
    pChr = pSvc->getCharacteristic("00002a24-0000-1000-8000-00805f9b34fb");
    if (pChr) {
        memcpy(model, pChr->readValue().c_str(), 99);
    }
    pChr = pSvc->getCharacteristic("00002a25-0000-1000-8000-00805f9b34fb");
    if (pChr) {
        memcpy(serial, pChr->readValue().c_str(), 99);
    }
    pChr = pSvc->getCharacteristic("00002a26-0000-1000-8000-00805f9b34fb");
    if (pChr) {
        memcpy(fw, pChr->readValue().c_str(), 99);
    }
    pChr = pSvc->getCharacteristic("00002a27-0000-1000-8000-00805f9b34fb");
    if (pChr) {
        memcpy(hw, pChr->readValue().c_str(), 99);
    }
    pChr = pSvc->getCharacteristic("00002a29-0000-1000-8000-00805f9b34fb");
    if (pChr) {
        memcpy(manufacturer, pChr->readValue().c_str(), 99);
    }
    Serial.printf("Connected to model \"%s\" - Serial \"%s\" - FW: \"%s\" - HW \"%s\" produced by %s\n", model, serial, fw, hw, manufacturer);
 
    // Subscribe to the data pipeline
    pSvc = pClient->getService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
    if (pSvc) {
        // RX setup data here
        /*
        pChr = pSvc->getCharacteristic("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
        if (pChr) {
             if (pChr->canWrite()) {
                if (pChr->writeValue("No tip!")) {
                    Serial.printf("Wrote new value to: %s\n", pChr->getUUID().toString().c_str());
                } else {
                    pClient->disconnect();
                    return false;
                }
            }
        }
        */
        // TX
        pChr = pSvc->getCharacteristic("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
        if (pChr) {
             if (pChr->canNotify()) {
                 if (!pChr->subscribe(true, notifyCB)) {
                     pClient->disconnect();
                     return false;
                 }
             }
         }
     } else {
         Serial.printf("Data service not found.\n");
     }
 
     Serial.printf("Done with this device!\n");
     return true;
 }
 

void onESPReceive(const esp_now_recv_info_t *recvInfo, const uint8_t *incomingData, int len) {
    if (len == 5) {
        uint32_t magic;
        memcpy(&magic, incomingData, 4);
        if (magic == ESPNOW_MAGIC) {
            uint8_t received_channel = incomingData[4];
            Serial.printf("Detected broadcast on channel %d\n", received_channel);

            if (!locked_channel) {
                locked_channel = true;
                current_channel = received_channel;
                WiFi.disconnect(); // Stop scanning
                WiFi.mode(WIFI_STA);
                esp_wifi_set_promiscuous(false);
                esp_wifi_set_channel(received_channel, WIFI_SECOND_CHAN_NONE);
                Serial.printf("Locked to channel %d\n", current_channel);
            }
        }
    }
}

void setupESPNOW() {
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESPNOW] Init Failed");
        return;
    }

    esp_now_register_recv_cb(onESPReceive);
}

 void setup() {
     Serial.begin(115200);
     Serial.printf("Starting NimBLE Client\n");
 
     /** Initialize NimBLE and set the device name */
     NimBLEDevice::init("NimBLE-Client");
 
     /**
      * Set the IO capabilities of the device, each option will trigger a different pairing method.
      *  BLE_HS_IO_KEYBOARD_ONLY   - Passkey pairing
      *  BLE_HS_IO_DISPLAY_YESNO   - Numeric comparison pairing
      *  BLE_HS_IO_NO_INPUT_OUTPUT - DEFAULT setting - just works pairing
      */
     // NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_ONLY); // use passkey
     // NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO); //use numeric comparison
 
     /**
      * 2 different ways to set security - both calls achieve the same result.
      *  no bonding, no man in the middle protection, BLE secure connections.
      *  These are the default values, only shown here for demonstration.
      */
     // NimBLEDevice::setSecurityAuth(false, false, true);
 
     NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);
 
     /** Optional: set the transmit power */
     NimBLEDevice::setPower(3); /** 3dbm */
     NimBLEScan* pScan = NimBLEDevice::getScan();
 
     /** Set the callbacks to call when scan events occur, no duplicates */
     pScan->setScanCallbacks(&scanCallbacks, false);
 
     /** Set scan interval (how often) and window (how long) in milliseconds */
     pScan->setInterval(100);
     pScan->setWindow(100);
 
     /**
      * Active scan will gather scan response data from advertisers
      *  but will use more energy from both devices
      */
     pScan->setActiveScan(true);
 
     /** Start scanning for advertisers */
     pScan->start(scanTimeMs);
     Serial.printf("Scanning for peripherals\n");
     WiFi.mode(WIFI_STA);
     setupESPNOW();
 }
 
 void loop() {
    /** Loop here until we find a device we want to connect to */
    delay(1000);


    if (doConnect) {
        doConnect = false;
        /** Found a device we want to connect to, do it now */
        if (connectToServer()) {
            Serial.printf("Success! we should now be getting notifications, scanning for more!\n");
        } else {
            Serial.printf("Failed to connect, starting scan\n");
            NimBLEDevice::getScan()->start(scanTimeMs, false, true);
        }
        //NimBLEDevice::getScan()->start(scanTimeMs, false, true);
    }
    if (!locked_channel && !doConnect) {
        for (uint8_t ch = 1; ch <= 13 && !locked_channel; ch++) {
            Serial.printf("Scanning channel %d\n", ch);
            esp_wifi_set_promiscuous(false);
            esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
            delay(CHANNEL_SCAN_INTERVAL_MS);
        }
    } else if (locked_channel) {
        Serial.printf("Locked to channel %d\n", current_channel);
    }
 }