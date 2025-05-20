#pragma once
#include <math.h>
#include <stdlib.h>
#include "protocol.h"
#include "racebox.h"

extern RaceboxDataMessage latestGPS;
extern CurrentVehicleData latestVehicle;

// ─── TRACK GEOMETRY (all in meters) ───────────────────────────────────────────
const float straightLen_m = 550.0f;    // each straight ~55 m
const float curveRadius_m = 280.0f;    // semicircle radius ~28 m
const float trackLength_m = 2*(straightLen_m + (float)M_PI*curveRadius_m);

// ─── EARTH CONVERSION ─────────────────────────────────────────────────────────
const float centerLat = 58.3690644f;
const float centerLon = 15.2868968f;
// approx meters per degree at that latitude
const float metersPerDegLat = 111320.0f;
const float metersPerDegLon = 111320.0f * cosf(centerLat * M_PI/180.0f);

// ─── SIMULATION STATE ────────────────────────────────────────────────────────
static float simTime    = 0.0f;      // seconds
static float pathDist_m = 0.0f;      // meters along the oval
static float coolantTemp = 80.0f, oilTemp = 110.0f;
bool hasFix = false;
extern bool hasCAN;

static void trackXY(float d, float &x, float &y) {
    // segment lengths
    const float L1 = straightLen_m;
    const float L2 = M_PI * curveRadius_m;
    const float L3 = straightLen_m;
    const float L4 = M_PI * curveRadius_m;
    // wrap
    float s = fmodf(d, L1+L2+L3+L4);

    if (s < L1) {
        // bottom straight (east)
        x = -L1/2 + s;  y = -curveRadius_m;
    }
    else if (s < L1 + L2) {
        // east semicircle, bottom→top via south
        float φ = -M_PI/2 + (s-L1)/curveRadius_m;  // –π/2→+π/2
        x =  L1/2 + curveRadius_m * cosf(φ);
        y =         curveRadius_m * sinf(φ);
    }
    else if (s < L1 + L2 + L3) {
        // top straight (west)
        float d3 = s - (L1 + L2);
        x =  L1/2 - d3;  y = +curveRadius_m;
    }
    else {
        // west semicircle, top→bottom via north
        float φ = M_PI/2 + (s - (L1+L2+L3))/curveRadius_m; // π/2→3π/2
        x = -L1/2 + curveRadius_m * cosf(φ);
        y =  curveRadius_m * sinf(φ);
    }
}

void simulateCarLoop(void* pvParams) {
    TickType_t simWakeTime = xTaskGetTickCount();
    while (true) {
        // ── 1) ADVANCE TIME ──────────────────────────────────────────────────
        const float dt = 0.01f;  // 10 ms
        simTime += dt;

        // ── 2) GPS SIMULATION UNTIL FIX ────────────────────────────────────
        if (!hasFix && false) {
            if (latestGPS.fixStatus == 3) {
                hasFix = true;
            } else {
                // a) choose speed based on segment (straight vs curve) + jitter
                float speedKmh = ((pathDist_m < straightLen_m) ||
                                  (pathDist_m >= straightLen_m + M_PI*curveRadius_m &&
                                   pathDist_m < 2*straightLen_m + M_PI*curveRadius_m))
                                 ? 80.0f : 50.0f;
                speedKmh += 5.0f * sinf(simTime * 0.3f);
                speedKmh = fmaxf(20.0f, fminf(speedKmh, 100.0f));

                // b) move along track by dist = v * dt
                float speedMps = speedKmh / 3.6f;
                float delta    = speedMps * dt;
                pathDist_m = fmodf(pathDist_m + delta, trackLength_m);

                // move along by v*dt (you already have pathDist_m updated)
                float x0, y0;
                trackXY(pathDist_m, x0, y0);

                // look 1 m ahead
                float ahead = fmodf(pathDist_m + 1.0f, trackLength_m);
                float x1, y1;
                trackXY(ahead, x1, y1);

                // true heading vector
                float headingRad = atan2f(y1 - y0, x1 - x0);

                // convert into compass heading (0°=North, clockwise)
                float headingDeg = 90.0f - headingRad * 180.0f / M_PI;
                if (headingDeg < 0) headingDeg += 360.0f;

                // write out
                latestGPS.latitude  = (uint32_t)((centerLat + y0/metersPerDegLat)  * 1e7f);
                latestGPS.longitude = (uint32_t)((centerLon + x0/metersPerDegLon) * 1e7f);
                latestGPS.heading   = (uint32_t)(headingDeg * 1e5f);
                latestGPS.speed   = (uint16_t)(speedMps * 1000.0f);
            }
        }

        // ── 3) CAN SIMULATION UNTIL REAL CAN ────────────────────────────────
        if (!hasCAN) {
            // mirror GPS speed on wheels
            float speedKmh = latestGPS.speed / 100.0f / 3.6;
            uint16_t ws    = (uint16_t)(speedKmh * 100.0f);
            latestVehicle.wheel_speed_FL = ws;
            latestVehicle.wheel_speed_FR = ws;
            latestVehicle.wheel_speed_RL = ws;
            latestVehicle.wheel_speed_RR = ws;

            // brake temps, heart rate, temps, controls, rpm & fuel...
            bool inCurve = (speedKmh < 60.0f);
            for (int i = 0; i < 16; ++i) {
                float noise = 5.0f * sinf(simTime*1.2f + i);
                float base  = 120.0f + (inCurve ? 40.0f : 0.0f) + noise;
                latestVehicle.brakeTemp[i] = (uint16_t)(base * 10.0f);
            }
            latestVehicle.heart_rate = 70 + (uint8_t)(inCurve*10 + 3*sinf(simTime*0.7f));

            // coolant & oil correlated
            const float tau=30.0f, meanC=90.0f, meanO=100.0f;
            coolantTemp += (meanC-coolantTemp)*(1-expf(-dt/tau));
            oilTemp     += (meanO-oilTemp)*(1-expf(-dt/tau));
            float dno = 0.2f*((rand()/(float)RAND_MAX)-0.5f);
            coolantTemp += dno;  oilTemp += dno*1.2f;
            latestVehicle.coolant_temp = (uint8_t)coolantTemp;
            latestVehicle.oil_temp     = (uint8_t)oilTemp;

            // controls
            latestVehicle.accelerator = (uint8_t)(30 + 20*sinf(simTime*0.5f));
            latestVehicle.brake       = (uint8_t)(inCurve*60.0f);
            latestVehicle.clutch      = 0;

            // gears & rpm
            uint8_t gear = (speedKmh>70?5:(speedKmh>50?4:3));
            static const float gearRatios[6]={0,150,100,75,60,50};
            float baseRpm = speedKmh * gearRatios[gear];
            float noiseRpm= (rand()%50)-25;
            float rawRpm  = baseRpm + 800.0f + noiseRpm;
            latestVehicle.rpm = (uint16_t)fmaxf(800.0f, fminf(rawRpm,7500.0f));

            // fuel drain
            latestVehicle.fuel_level = 6330 - (uint16_t)(simTime*0.5f);
        }

        // 3.5) quit if not needed
        if (true /* hasCAN && hasFix */) {
            Serial.printf("We have GPS fix and CAN-data. Fake data no longer needed.\n");
            vTaskDelete(NULL);
        }
        // ── 4) WAIT ──────────────────────────────────────────────────────────
        vTaskDelayUntil(&simWakeTime, pdMS_TO_TICKS(10));
    }
}
