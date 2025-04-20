#pragma once
#include <math.h>
#include "protocol.h"
#include "racebox.h"

extern RaceboxDataMessage latestGPS;
extern CurrentVehicleData latestVehicle;


// Simple fake track params
float angle = 0.0f;
const float radius = 0.0010f; // approx 11m radius loop
const float centerLat = 58.3690644;
const float centerLon = 15.2868968;
uint16_t rpm = 3000;
bool hasFix = false;
bool hasCAN = false;
TickType_t simWakeTime = xTaskGetTickCount();
void simulateCarLoop(void* pvParams) {
	float simTime = 0.0f;
	float speedKmh = 30.0f;
	float brakePhase = 0.0f;
	uint8_t gear = 2;

	while (true) {
        if (!hasFix) {
            if(latestGPS.fixStatus == 3) {
                hasFix = true;
            }
            // Simulate speed between 20–70 km/h
            speedKmh = 45.0f + 15.0f * sin(simTime * 0.5f);
            latestGPS.speed = (uint16_t)(speedKmh * 100);

            // Heading changes like turning on a small track
            float headingDeg = fmod(simTime * 20.0f, 360.0f);
            latestGPS.heading = headingDeg * 10e4; // degrees with a factor of 10^5, north is 0

            // Positional data (lat/lon around small loop)
            float t = simTime;
            float offset = t * 0.5f;
            float lat = centerLat + radius * cos(offset);
            float lon = centerLon + radius * sin(offset);
            latestGPS.latitude = (uint32_t)(lat*1e7);
            latestGPS.longitude = (uint32_t)(lon*1e7);
        }
        if(!hasCAN) {
            // Simulate speed between 20–70 km/h
			speedKmh = 45.0f + 15.0f * sin(simTime * 0.5f);
			latestVehicle.wheel_speed_FL = (uint16_t)(speedKmh * 100);
            latestVehicle.wheel_speed_FR = (uint16_t)(speedKmh * 100);
            latestVehicle.wheel_speed_RL = (uint16_t)(speedKmh * 100);
            latestVehicle.wheel_speed_RR = (uint16_t)(speedKmh * 100);

			// Brake temps oscillate a bit, higher during braking phase
			brakePhase = 0.5f + 0.5f * sin(simTime * 0.8f);
			for (int i = 0; i < 16; ++i) {
				float noise = sin(simTime * 2.0f + i) * 1.5f;
				float base = 120.0f + brakePhase * 30.0f + noise;
				latestVehicle.brakeTemp[i] = (uint16_t)((base + 100.0f) * 10.0f);
			}

			// Heart rate fluctuates slightly
			latestVehicle.heart_rate = 78 + (uint8_t)(3 * sin(simTime * 0.7f));

			// Coolant & oil temp increase slowly, then stabilize
			float coolant = 70.0f + 25.0f * (1 - exp(-simTime / 30.0f));
			float oil = 80.0f + 30.0f * (1 - exp(-simTime / 45.0f));
			latestVehicle.coolant_temp = (uint8_t)coolant;
			latestVehicle.oil_temp = (uint8_t)oil;

			// Throttle input varies with sine wave
			latestVehicle.accelerator = (uint8_t)(40 + 30 * sin(simTime * 0.6f));
			latestVehicle.clutch = 0;
			latestVehicle.brake = (uint8_t)(brakePhase * 50.0f);

			// RPM = speed (m/s) * gear ratio approximation
			float speedMps = speedKmh / 3.6f;
			uint16_t rpmVal = (uint16_t)(speedMps * gear * 250.0f + (rand() % 100));
			latestVehicle.rpm = rpmVal;

            latestVehicle.fuel_level = 1337;
        }
		simTime += 0.01f;
		vTaskDelayUntil(&simWakeTime, pdMS_TO_TICKS(10));
	}
}

