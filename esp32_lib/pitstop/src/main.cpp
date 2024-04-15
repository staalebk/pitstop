#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <time.h>
#include "pitstop.h"
#include "configmanager.h"


const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;         // GMT offset in seconds
const int   daylightOffset_sec = 0; // Adjust according to your daylight saving settings



uint16_t localUdpPort = 1337;  // Local port to listen on
const char * udpAddress = "85.166.206.94";  // IP address for UDP (could be a broadcast address)
uint16_t udpPort = 5005;  // Port for UDP data

String jsonUrl = "http://yourjsonurl.com/data.json";  // URL to fetch JSON data

void setup() {
  sleep(3);
  Serial.println("Booting...");
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("W");
  }
  Serial.println("Connected to Wi-Fi");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  setupPitstop(udpAddress, udpPort, localUdpPort);
}

void fetchAndProcessJson() {
  HTTPClient http;
  http.begin(jsonUrl);  // Specify the URL
  int httpCode = http.GET();  // Make the request

  if (httpCode > 0) {  // Check for the returning code
    String payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);
    // Here you can parse the payload using JSON library if needed
    // For example, to extract information and make decisions on what data to send via UDP
  } else {
    Serial.println("Error on HTTP request");
  }

  http.end();  // Free resources
}

void loop() {
  //fetchAndProcessJson();  // Fetch and process JSON data

  // Example sending UDP packet based on fetched data
  /*
  udp.beginPacket(udpAddress, udpPort);
  udp.write();
  udp.write("Data based on JSON");  // Replace with actual data from JSON if applicable
  udp.endPacket();
  Serial.println("UDP packet sent based on JSON data");

 */
  sendAuth();
  delay(10000); // Wait for 10 seconds before the next loop iteration
}