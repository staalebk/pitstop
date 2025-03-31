#include "protocol.h"
#include <math.h>

// Shared simulated packet
DataPacket simulatedPacket;
SemaphoreHandle_t packetMutex;

// Simple fake track params
float angle = 0.0f;
const float radius = 0.0010f; // approx 11m radius loop
const float centerLat = 58.3690644;
const float centerLon = 15.2868968;
uint16_t rpm = 3000;
TickType_t simWakeTime = xTaskGetTickCount();
void simulateCarLoop(void* pvParams) {
	float simTime = 0.0f;
	float speedKmh = 30.0f;
	float brakePhase = 0.0f;
	uint8_t gear = 2;

	while (true) {
		if (xSemaphoreTake(packetMutex, portMAX_DELAY)) {
			simulatedPacket.protocolVersion = 0x00;
			simulatedPacket.timestamp = ((int64_t)micros() / 100000) * 100000;

			// Simulate speed between 20–70 km/h
			speedKmh = 45.0f + 15.0f * sin(simTime * 0.5f);
			simulatedPacket.vehicleData.speed = (uint16_t)(speedKmh * 100);

			// Heading changes like turning on a small track
			float headingDeg = fmod(simTime * 20.0f, 360.0f);
			simulatedPacket.vehicleData.heading = (uint16_t)(headingDeg * 100);

			// Brake temps oscillate a bit, higher during braking phase
			brakePhase = 0.5f + 0.5f * sin(simTime * 0.8f);
			for (int i = 0; i < 16; ++i) {
				float noise = sin(simTime * 2.0f + i) * 1.5f;
				float base = 120.0f + brakePhase * 30.0f + noise;
				simulatedPacket.vehicleData.brakeTemp[i] = (uint16_t)((base + 100.0f) * 10.0f);
			}

			// Heart rate fluctuates slightly
			simulatedPacket.vehicleData.heart_rate = 78 + (uint8_t)(3 * sin(simTime * 0.7f));

			// Coolant & oil temp increase slowly, then stabilize
			float coolant = 70.0f + 25.0f * (1 - exp(-simTime / 30.0f));
			float oil = 80.0f + 30.0f * (1 - exp(-simTime / 45.0f));
			simulatedPacket.vehicleData.coolant_temp = (uint8_t)coolant;
			simulatedPacket.vehicleData.oil_temp = (uint8_t)oil;

			// Throttle input varies with sine wave
			simulatedPacket.vehicleData.accelerator = (uint8_t)(40 + 30 * sin(simTime * 0.6f));
			simulatedPacket.vehicleData.clutch = 0;
			simulatedPacket.vehicleData.brake = (uint8_t)(brakePhase * 50.0f);

			// Positional data (lat/lon around small loop)
			for (int i = 0; i < POS_PER_PACKET; ++i) {
				float t = simTime - (i * 0.1f);
				float offset = t * 0.5f;
				float lat = centerLat + radius * cos(offset);
				float lon = centerLon + radius * sin(offset);
				simulatedPacket.positions[i].latitude = (uint32_t)(lat * 6000000.0f);
				simulatedPacket.positions[i].longitude = (uint32_t)(lon * 6000000.0f);

				// RPM = speed (m/s) * gear ratio approximation
				float speedMps = speedKmh / 3.6f;
				uint16_t rpmVal = (uint16_t)(speedMps * gear * 250.0f + (rand() % 100));
				simulatedPacket.positions[i].rpm = rpmVal;
			}

			xSemaphoreGive(packetMutex);
		}

		simTime += 0.01f;
		vTaskDelayUntil(&simWakeTime, pdMS_TO_TICKS(10)); // 100 ms intervals
	}
}

