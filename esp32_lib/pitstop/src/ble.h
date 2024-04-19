#pragma once
#include<Arduino.h>

#define CMD_TYPE_REMOVE_ALL 0
#define CMD_TYPE_REMOVE 1
#define CMD_TYPE_ADD_INCOMPLETE 2
#define CMD_TYPE_ADD 3
#define CMD_TYPE_UPDATE_ALL 4
#define CMD_TYPE_UPDATE 5

#define CMD_RESULT_OK 0
#define CMD_RESULT_PAYLOAD_OUT_OF_SEQUENCE 1
#define CMD_RESULT_EQUATION_EXCEPTION 2

#define MAX_REMAINING_PAYLOAD 2048
#define MAX_PAYLOAD_PART 17
#define MONITOR_NAME_MAX 32
#define MONITORS_MAX 10

#define DATA_LAT 1
#define DATA_LON 2
#define DATA_SPEED 3
#define DATA_RPM 4
#define DATA_HEADING 5
#define DATA_TIMESTAMP 6
#define DATA_HRM 7

typedef void (*CallbackType)(uint32_t);      // Function pointer type for callbacks

void registerCallback(int id, CallbackType callback);
int32_t getData(int data);
void setupRCBle(const char *name);
void setupRCBle(uint8_t *UUID);
void pollRCBle();