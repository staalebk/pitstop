/*
  Copyright (c) [2024] [Vincent Kratzer]

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  This code is licensed under the GPL-3.0 License.
*/

//Required libraries: NimBLEclient

#include <NimBLEDevice.h>

//################### important setting #########################

//example how to define a certain address so that we only connect to a distinguished RaceBox that matches this address:   #define TARGET_DEVICE_ADDRESS "aa:bb:cc:dd:ee:ff"
//replace the actual address below with the one that matches your device (you will see the address printed in the serial output of this code while it is scanning for bluetooth devices on start)

//#define TARGET_DEVICE_ADDRESS "aa:bb:cc:dd:ee:ff"

//alternatively, you may comment the line above like this: //#define TARGET_DEVICE_ADDRESS "aa:bb:cc:dd:ee:ff".
//--> In that case the code will connect to any RaceBox (or to be clear: to any device that advertises a name beginning with 'RaceBox'), as long as the advertised service UUID also matches the requirement.

//##############################################################


//device type, will be set automatically:  0: RaceBox Mini/Mini S, 1: RaceBox Micro - used to handle different battery status decoding between racebox mini/mini s and micro.
int deviceType = -1; //-1: unknown device type as default, the statement 'if (deviceName.rfind("RaceBox Micro", 0) == 0) {' and the following lines in class AdvertisedDeviceCallbacks automatically determines the device type.

// BLE UUIDs
static BLEUUID UART_service_UUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID RX_characteristic_UUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E"); //currently not used in this code example
static BLEUUID TX_characteristic_UUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID NMEA_UART_service_UUID("00001101-0000-1000-8000-00805F9B34FB"); //currently not used in this code example
static BLEUUID NMEA_RX_characteristic_UUID("00001102-0000-1000-8000-00805F9B34FB"); //currently not used in this code example
static BLEUUID NMEA_TX_characteristic_UUID("00001103-0000-1000-8000-00805F9B34FB"); //currently not used in this code example
//static BLEUUID Serial_Number_Characteristic_UUID ("00002a25-0000-1000-8000-00805f9b34fb"); //individual serial number UUID - currently not used in this code example


// Configuration
const int outputFrequencyHzSerial = 8; //in Hz
const unsigned long outputIntervalMs_serial = 1000 / outputFrequencyHzSerial; 

static bool doConnect = false;
static bool connected = false;
static bool doScan = false;
static bool updated_RaceBox_Data_Message = false; //used to determine if we have new live data to print
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myRaceBox;

unsigned long lastOutputTimeSerial = 0;

//global variables for live data from RaceBox (at 25Hz): (examples see function void parsePayload)
uint16_t header;
uint8_t messageClass;
uint8_t messageId;
uint16_t payloadLength;
uint32_t iTOW;
uint16_t year;
uint8_t month;
uint8_t day;
uint8_t hour;
uint8_t minute;
uint8_t second;
uint8_t validityFlags;
uint32_t timeAccuracy;
uint32_t nanoseconds;
uint8_t fixStatus;
uint8_t fixStatusFlags;
uint8_t dateTimeFlags;
uint8_t numSVs;
int32_t longitude;
int32_t latitude;
int32_t wgsAltitude;
int32_t mslAltitude;
uint32_t horizontalAccuracy;
uint32_t verticalAccuracy;
uint32_t speed;
uint32_t heading;
uint32_t speedAccuracy;
uint32_t headingAccuracy;
uint16_t pdop;
uint8_t latLonFlags;
uint8_t batteryStatus;
int16_t gForceX;
int16_t gForceY;
int16_t gForceZ;
int16_t rotRateX;
int16_t rotRateY;
int16_t rotRateZ;

float headingDegrees;


// Callback for notifications (updated syntax remains the same)
static void notifyCallback(NimBLERemoteCharacteristic* pCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    if (length >= 80) {
        parsePayload(pData);
    } else {
        Serial.println("Payload shorter than 80 bytes.");
        parsePayload(pData);
    }
}

// Updated AdvertisedDeviceCallbacks class
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* advertisedDevice) override {
    Serial.print("Advertised BLE Device found: ");
    Serial.println(advertisedDevice->toString().c_str());

    if (advertisedDevice->isAdvertisingService(UART_service_UUID)) {
      std::string deviceName = advertisedDevice->getName();
      if (deviceName.rfind("RaceBox", 0) == 0) {

        if (deviceName.rfind("RaceBox Micro", 0) == 0) deviceType = 1;
        else if (deviceName.rfind("RaceBox Mini", 0) == 0) deviceType = 0;
        else deviceType = -1;

#ifdef TARGET_DEVICE_ADDRESS
        if (advertisedDevice->getAddress().toString() == TARGET_DEVICE_ADDRESS) {
          NimBLEDevice::getScan()->stop();
          myRaceBox = advertisedDevice;
          doConnect = true;
        } else {
          Serial.println("Address mismatch. Not connecting.");
        }
#else
        Serial.println("RaceBox found. Connecting...");
        NimBLEDevice::getScan()->stop();
        myRaceBox = advertisedDevice;
        doConnect = true;
#endif
      }
    }
  }
};


//decode battery status
void decodeBatteryStatus(uint8_t batteryStatus) {
    if (deviceType == 0) { //RaceBox Mini or RaceBox Mini S
        bool isCharging = (batteryStatus & 0x80) != 0; //check if the MSB is set (charging status)
        uint8_t batteryLevel = batteryStatus & 0x7F;   //battery level (remaining 7 bits)
        Serial.print("RaceBox Mini/Mini S - "); //interpreting battery status according to datasheet
        Serial.print("Charging Status: ");
        Serial.println(isCharging ? "Charging" : "Not Charging");
        Serial.print("Battery Level: ");
        Serial.print(batteryLevel);
        Serial.println("%");
    } else if (deviceType == 1) { //RaceBox Micro
        Serial.print("RaceBox Micro - "); //interpreting battery status as input voltage (according to datasheet)
        float inputVoltage = batteryStatus / 10.0; //Input voltage must be multiplied by 10, according to datasheet
        Serial.print("Input Voltage: ");
        Serial.print(inputVoltage, 1); //print with one decimal
        Serial.println(" V");
    } else {
        Serial.println("Battery status: Unknown device type");
    }
}

void calculateChecksum(uint8_t* data, uint16_t length, uint8_t& CK_A, uint8_t& CK_B) {
    CK_A = 0;
    CK_B = 0;
    for (int i = 2; i < length - 2; i++) { //start after header bytes and end before checksum bytes
        CK_A += data[i];
        CK_B += CK_A;
    }
}


void parsePayload(uint8_t* data) {

    //check for correct frame start - may need to be removed or changed for other data than RaceBox Data Message!
    if (data[0] != 0xB5 || data[1] != 0x62) {
        Serial.println("Invalid frame start of payload data - check may need to be removed or changed for other data than RaceBox Data Message!");
        return;
    }

    
//Extract data from payload, at first we need the payloadLength to calculate the checksum
                                                                    //examples for data content, mostly copied from RaceBox datasheet:
    header = *(reinterpret_cast<uint16_t*>(data));                  //0xB5 0x62 (that are the two header identification bytes, according to RaceBox datasheet: The first 2 bytes are the frame start - always 0xB5 and 0x62.)
    messageClass = *(reinterpret_cast<uint8_t*>(data + 2));         //expecting 0xFF for a RaceBox data message
    messageId = *(reinterpret_cast<uint8_t*>(data + 3));            //expecting 0x01 for a RaceBox data message (equals 0x1)
    payloadLength = *(reinterpret_cast<uint16_t*>(data + 4));       //e.g. 0x50 0x00 (80 bytes for live data - attention: other data is larger (up to 509 bytes), divided into multiple packets and needs to be reassembled from multiple packets - refer to datasheet)


    //validate the length of the packet
    uint16_t packetLength = 6 + payloadLength + 2; //header (6 bytes) + payload + checksum (2 bytes)
    if (packetLength > 512) { //double check if 5
        Serial.print("Received packet size exceeds maximum allowed size (512 bytes). ");
        Serial.print("Packet length is ");
        Serial.print(packetLength);
        Serial.println(" bytes.");
        return;
    } else { //if packetLength is within allowed limits, print out some info on the data packet:
        //Serial.println("Payload length is " + String(payloadLength) + " bytes");
        //Serial.println("Expected payload length according to datasheet: 0 - 504 bytes. For a RaceBox Data Message payload length is 80 bytes.");
        //Serial.println("Packet length (including checksum) is " + String(packetLength) + " bytes");
    }
    
    //validate checksum
    uint8_t CK_A, CK_B;
    calculateChecksum(data, packetLength, CK_A, CK_B);
    if (data[packetLength - 2] != CK_A || data[packetLength - 1] != CK_B) {
        Serial.println("*** Checksum validation of incoming data package failed. ***");
        return;
    } else {
        //Serial.println("Checksum validation successful.");
    }
    //Serial.println();

    //print message class and message ID - used to determine the type of message. A RaceBox Data Message has messageClass 0xFF and messageId 0x01.
//    Serial.print("Message Class: 0x");
//    Serial.println(messageClass, HEX);
//    Serial.print("Message ID: 0x");
//    Serial.println(messageId, HEX);

    //check if the message class and ID match the expected values for a live data packet
    if (messageClass == 0xFF || messageId == 0x01) {//in case we receive live data (standard on start of RaceBox) and interpret it accordingly
      //Serial.println("the received message has messageClass 0xFF and messageId 0x01, this is a (valid) RaceBox Data Message. Parsing payload.");
      parse_RaceBox_Data_Message_payload(data); //sending variable data to this function to interpret it
      //outputting received data (this will be triggered each time a payload is parsed, so be aware that it may delay data update rate if e.g. printing a lot of info to serial takes longer than it takes for the next data to arrive.)
//    print_RaceBox_Data_message_payload_to_oled(); 
//      
//      //####### be aware that those serial outputs below are sent each time a payload is parsed (i.e. up to 25x per second) like an interrupt, so the serial output from this function can overlay/interfere with output from void loop ####### 
//    print_RaceBox_Data_message_payload_to_serial(); //<-- ATTENTION! see comment above
//      //if you move this function to loop, you may want to comment out all serial outputs from void parsePayload.
    }
    
    //examples how to handle other received messages;
    else if (messageClass == 0xFF || messageId == 0x21) {//History Data Message
      Serial.println("the received message has messageClass 0xFF and messageId 0x21, this is a (valid) History Data Message message. Parsing payload NOT yet implemented.");
      //parse_History_Data_Message_payload(data); //sending variable data to this function to interpret it  (function not yet implemented)
    }
    else if (messageClass == 0xFF || messageId == 0x22) {//Standalone Recording Status
      Serial.println("the received message has messageClass 0xFF and messageId 0x22, this is a (valid) Standalone Recording Status message. Parsing payload NOT yet implemented.");
      //parse_standalone_Recording_Status_payload(data); //sending variable data to this function to interpret it  (function not yet implemented)
    }
    else if (messageClass == 0xFF || messageId == 0x23) {//Recorded Data Download
      Serial.println("the received message has messageClass 0xFF and messageId 0x23, this is a (valid) Recorded Data Download message. Parsing payload NOT yet implemented.");
      //parse_Recorded_Data_payload(data); //sending variable data to this function to interpret it  (function not yet implemented)
    }
    else if (messageClass == 0xFF || messageId == 0x26) {//Standalone Recording State Change Message
      Serial.println("the received message has messageClass 0xFF and messageId 0x26, this is a (valid) Standalone Recording State Change Message. Parsing payload NOT yet implemented.");
      //parse_Recorded_Data_payload(data); //sending variable data to this function to interpret it  (function not yet implemented)
    }
    
//    else if (messageClass == 0x_something_else_1 || messageId == 0x_something_else_2){
//      //handle other message class(es) like this
//    }

    else{ //in case we receive different data (with different message class or message IDs as implemented above, we would need to handle it differently, or even assemble multiple messages that may have ben split.
      Serial.print("unknown message class and message ID found (it may be other data?): ");
      Serial.print("Message Class: 0x");
      Serial.print(messageClass, HEX);
      Serial.print(", Message ID: 0x");
      Serial.println(messageId, HEX);       
      Serial.println("Ignoring packet. This is not a known/implemented data packet. Interpreting the payload for this kind of packet is not yet implemented.");
      return;
    }

    

}



//functions to interpret payload of different messages:


void parse_RaceBox_Data_Message_payload(uint8_t* data){ //function to handle payload of a RaceBox Data Message
    //writing updated values (from payload) to the variables:
    iTOW = *(reinterpret_cast<uint32_t*>(data + 6));                //e.g 0xA0 0xE7 0x0C 0x07
    year = *(reinterpret_cast<uint16_t*>(data + 10));               //e.g 0xE6 0x07 (2022) or 0xE8 0x07 (2024)
    month = *(reinterpret_cast<uint8_t*>(data + 12));               //0x01 (january) or 0x08 (august)
    day = *(reinterpret_cast<uint8_t*>(data + 13));                 //0x0A (10th) or 0x08 (8th)
    hour = *(reinterpret_cast<uint8_t*>(data + 14));                //0x08 (08 o'clock)
    minute = *(reinterpret_cast<uint8_t*>(data + 15));              //0x33 (51 min)
    second = *(reinterpret_cast<uint8_t*>(data + 16));              //0x08 (08 seconds)
    validityFlags = *(reinterpret_cast<uint8_t*>(data + 17));       //0x37 (Date/Time valid)
    timeAccuracy = *(reinterpret_cast<uint32_t*>(data + 18));       //0x19000000 (25 ns)
    nanoseconds = *(reinterpret_cast<uint32_t*>(data + 22));        //0x2AAD4D0E (239971626 ns = 0.239 seconds)
    fixStatus = *(reinterpret_cast<uint8_t*>(data + 26));           //0x03 (3D Fix)
    fixStatusFlags = *(reinterpret_cast<uint8_t*>(data + 27));      //0x01 (GNSS Fix OK)
    dateTimeFlags = *(reinterpret_cast<uint8_t*>(data + 28));       //0xEA (Date/Time Confirmed)
    numSVs = *(reinterpret_cast<uint8_t*>(data + 29));              //0x0B (11 satellites)
    longitude = *(reinterpret_cast<int32_t*>(data + 30));           //0xC693E10D (23.2887238 degrees)
    latitude = *(reinterpret_cast<int32_t*>(data + 34));            //0x3B376F19 (42.6719035 degrees)
    wgsAltitude = *(reinterpret_cast<int32_t*>(data + 38));         //0x618C0900 (625.761 meters)
    mslAltitude = *(reinterpret_cast<int32_t*>(data + 42));         //0x0F010900 (590.095 meters)
    horizontalAccuracy = *(reinterpret_cast<uint32_t*>(data + 46)); //0x9C030000 (0.924 meters)
    verticalAccuracy = *(reinterpret_cast<uint32_t*>(data + 50));   //0x2C070000 (1.836 meters)
    speed = *(reinterpret_cast<uint32_t*>(data + 54));              //0x23000000 (35 mm/s = 0.126 km/h)
    heading = *(reinterpret_cast<uint32_t*>(data + 58));            //0x00000000 (0 degrees)
    speedAccuracy = *(reinterpret_cast<uint32_t*>(data + 62));      //0xD0000000 (208 mm/s = 0.704 km/h)
    headingAccuracy = *(reinterpret_cast<uint32_t*>(data + 66));    //0x88A9DD00 (145.26856 degrees)
    pdop = *(reinterpret_cast<uint16_t*>(data + 70));               //0x2C01 (3)
    latLonFlags = *(reinterpret_cast<uint8_t*>(data + 72));         //0x00 (Coordinates valid)
    batteryStatus = *(reinterpret_cast<uint8_t*>(data + 73));       //has to be interpreted depending on if it is a RaceBox micro or mini, see my function void decodeBatteryStatus
    gForceX = *(reinterpret_cast<int16_t*>(data + 74));             //0xFDFF (-0.003 g)
    gForceY = *(reinterpret_cast<int16_t*>(data + 76));             //0x7100 (0.113 g)
    gForceZ = *(reinterpret_cast<int16_t*>(data + 78));             //0xCE03 (0.974 g)
    
    headingDegrees = heading / 100000.0; //convert it to a float variable that is needed for the function getCompassDirection
    updated_RaceBox_Data_Message = true; //bool is used to determine if updated data for variables in RaceBox Data Message is available (e.g. to print or display them in void loop() )
}


// Updated connectToRaceBox function
bool connectToRaceBox() {
  NimBLEClient* pClient = nullptr;

  if (NimBLEDevice::getClientListSize() > 0) {
    pClient = NimBLEDevice::getClientByPeerAddress(myRaceBox->getAddress());
    if (pClient && !pClient->connect(myRaceBox)) {
      Serial.println("Reconnect failed.");
      return false;
    }
  }

  if (!pClient) {
    pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(new ClientCallbacks(), false);
    if (!pClient->connect(myRaceBox)) {
      Serial.println("Connection failed.");
      NimBLEDevice::deleteClient(pClient);
      return false;
    }
  }

  BLERemoteService* pService = pClient->getService(UART_service_UUID);
  if (pService) {
    pRemoteCharacteristic = pService->getCharacteristic(TX_characteristic_UUID);
    if (pRemoteCharacteristic) {
      pRemoteCharacteristic->subscribe(true, notifyCallback);
      return true;
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  Serial.flush();
  Serial.println();
  delay(100);
  Serial.println();
  Serial.println();
  Serial.println("ESP32 Pitstop race hub");
  Serial.println();
  #ifdef TARGET_DEVICE_ADDRESS
   Serial.printf("Waiting for a specific RaceBox with TARGET_DEVICE_ADDRESS %s to appear (as set in code)...\n", TARGET_DEVICE_ADDRESS); //notice the Serial.printf to be able to print TARGET_DEVICE_ADDRESS to serial.
  #else
   Serial.println("Waiting for any RaceBox to appear (no specific TARGET_DEVICE_ADDRESS is set)...");
  #endif
  
  Serial.println();
  Serial.println("---------------------------------------------  scan results  --------------------------------------------------------");
  Serial.println();
  
  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setCallbacks(new AdvertisedDeviceCallbacks());
  pScan->setInterval(45);
  pScan->setWindow(15);
  pScan->setActiveScan(true);
  pScan->start(0, false);
  
}

void print_RaceBox_Data_message_payload_to_serial(){
  //serial print the received data:
    unsigned long currentTime = millis();
    if (currentTime - lastOutputTimeSerial >= outputIntervalMs_serial) { //limits the amount how often we print current values to serial

        // Serial output with correct formatting - HINT: the serial output as well as excessive updating of the OLED will take time and can hinder fast operation (e.g. reading in at 25hz), so output should be limited
        Serial.println();
        Serial.println("--- updated values from RaceBox Data Message available: ---");
        Serial.println("--------------------------------------------------------------------------------------------------");
        Serial.println("iTOW: " + String(iTOW) + " ms");
        Serial.println("Year: " + String(year));
        Serial.println("Month: " + String(month));
        Serial.println("Day: " + String(day));
        
        //Serial.println("Time (UTC): " + String(hour) + ":" + String(minute) + ":" + String(second));
        char timeString[9];  // Buffer to store the formatted time string
        sprintf(timeString, "%02d:%02d:%02d", hour, minute, second); //build a time string that always has the time format 00:00:00
        Serial.println("Time (UTC): " + String(timeString));
        
        //output fix status with interpretation
        String fixStatusText;
        if (fixStatus == 0) {
            fixStatusText = "No Fix";
        } else if (fixStatus == 2) {
            fixStatusText = "2D Fix";
        } else if (fixStatus == 3) {
            fixStatusText = "3D Fix";
        } else {
            fixStatusText = "Unknown";
        }
        Serial.println("GPS: " + fixStatusText);
        
        Serial.println("Satellites: " + String(numSVs));
        Serial.println("Latitude: " + String(latitude / 1e7, 7) + " deg"); //we need to divide the latitude by 10^7 because the datasheet states that it is transmitted with a factor of 10^7
        Serial.println("Longitude: " + String(longitude / 1e7, 7) + " deg");
        Serial.println("WGS Altitude: " + String(wgsAltitude / 1000.0, 2) + " m");
        Serial.println("MSL Altitude: " + String(mslAltitude / 1000.0, 2) + " m");
        Serial.println("Horizontal Accuracy: " + String(horizontalAccuracy / 1000.0, 2) + " m");
        Serial.println("Vertical Accuracy: " + String(verticalAccuracy / 1000.0, 2) + " m");
        Serial.println("Speed Accuracy: " + String(speedAccuracy / 1000.0, 2) + " m/s");
        Serial.println("Speed: " + String(speed / 1000.0, 2) + " m/s");
        Serial.println("Speed: " + String(speed*3.6 / 1000.0, 2) + " km/h");
        Serial.print("Heading Accuracy: " + String(headingAccuracy / 1e5, 1) + " deg");
        Serial.println(" (heading " + String((fixStatusFlags & 0x20) ? "valid)" : "NOT valid - may need movement to become valid)"));
        //Serial.print("Heading: " + String(heading / 1e5, 1) + " deg");
        Serial.print("Heading: ");
        Serial.print(headingDegrees, 1); //heading (one decimal)
        Serial.println("PDOP: " + String(pdop / 100.0, 2));
        Serial.println("G-Force X: " + String(gForceX / 1000.0, 3) + " G");
        Serial.println("G-Force Y: " + String(gForceY / 1000.0, 3) + " G");
        Serial.println("G-Force Z: " + String(gForceZ / 1000.0, 3) + " G");
        Serial.println("Rot Rate X: " + String(rotRateX / 100.0, 2) + " deg/s");
        Serial.println("Rot Rate Y: " + String(rotRateY / 100.0, 2) + " deg/s");
        Serial.println("Rot Rate Z: " + String(rotRateZ / 100.0, 2) + " deg/s");


        
        //print fix status flags
//        Serial.println("Fix Status Flags (Hex): " + String(fixStatusFlags, HEX));
//        Serial.println("Fix Status Flags (Binary): " + String(fixStatusFlags, BIN));

        //print fix status flags with interpretation
        Serial.println("Fix Status Flags Interpretation:");
        Serial.println("  Bit 0: Valid Fix: " + String((fixStatusFlags & 0x01) ? "Yes" : "No"));
        Serial.println("  Bit 1: Differential Corrections Applied: " + String((fixStatusFlags & 0x02) ? "Yes" : "No"));
        Serial.println("  Bits 4..2: Power State: " + String((fixStatusFlags >> 2) & 0x07));
        Serial.println("  Bit 5: Valid Heading: " + String((fixStatusFlags & 0x20) ? "Yes" : "No"));
        Serial.println("  Bits 7..6: Carrier Phase Range Solution: " + String((fixStatusFlags >> 6) & 0x03));
        Serial.println();
                
        //Serial.println("Battery Status: " + String(batteryStatus)); //needs a function for interpretation, which is depending on device type:
        decodeBatteryStatus(batteryStatus); //a separate decoding funtion is a better solution, as there are differences in interpretation depending if it is a racebox mini, mini s oder micro
        Serial.println();
        
    }
    else{
      Serial.println("skipping serial output due to set serial update limitation");
    }
    Serial.println("--------------------------------------------------------------------------------------------------");
    Serial.println();
}


void interpret_serial_input(){ //read from serial console input to control starting of functions
// Check if data is available in the Serial buffer
//Serial.println("ready to receive serial messages from console");
  if (Serial.available() > 0) {
    // Read the incoming byte
    char incomingByte = Serial.read();

    // Respond based on the received byte
    switch (incomingByte) {
      case '1':
        Serial.println("received the message '1'. Starting function1...");
        break;
      case '2':
        Serial.println("received the message '2'. Starting function2...");
        break;
      case '3':
        Serial.println("received the message '3'. Starting function3...");
        break;
      default:
        Serial.println("Invalid input. Please enter '1', '2', or '3'.");
        break;
    }
  }
}

void loop() {
  if (doConnect) { //if we have requested to connect to a RaceBox
    //Serial.println("DEBUG: doConnect = true (in void loop) - trying to (re)connect...");
    if (connectToRaceBox()) {
      Serial.println("successfully connected to RaceBox.");
      Serial.println();
      //stop scanning for Bluetooth devices, now that we're connected to our RaceBox
        NimBLEDevice::getScan()->stop();
    } else {
      Serial.println("Failed to connect to RaceBox. Reattempting BLE connection...");
      //NimBLEDevice::getScan()->start(0, false); //scan indefinitely (0) until we stop it manually  //-->this is a simple restart with known scanning parameters
      //better approch: make sure scanning setup is correct, although slightly more complex:
      NimBLEScan* pScan = NimBLEDevice::getScan();
      pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
      pScan->setInterval(45);
      pScan->setWindow(15);
      pScan->setActiveScan(true);
      pScan->start(0, false); //scan indefinitely until we stop it manually
    }
    doConnect = false;
  }
  if (connected) { //if we are connected to RaceBox
    //add your code here
    interpret_serial_input(); //this function listens to serial console inputs from your computer (when a RaceBox is connected, due to the check 'if (connected)'. Sending 1, 2 or 3 will start functions (currently only empty function prototypes are implemented to give a starting point)
    if(updated_RaceBox_Data_Message==true){ //if we have received updated values from a RaceBox data message
      print_RaceBox_Data_message_payload_to_serial();
      updated_RaceBox_Data_Message=false; //reset bool
    }
  }
  else{
    //do something else if not connected to RaceBox
    interpret_serial_input(); //just for debugging purposes, we also interpret serial input here. Can be removed if we only want to start functions that e.g. send data to RaceBox, but can be useful for development so that the functions are called even if no racebox is connected.
  }

 //All BT handling and some output (to serial and OLED SPI display) is triggered and done separately each time data comes in.
 //ATTENTION: the serial output from those functions can interfere (timewise) with serial output from loop as they are sent each time a packet comes in. you may want to deactivate those serial outputs of received data, to not disturb any serial output that you add to void loop.
}