#ifndef OBD2_H
#define OBD2_H

#include <stdint.h>
#include "can.h"

// Standard 11-bit CAN OBD-II IDs
#define OBD2_CAN_REQ_ID         0x7DFU
#define OBD2_CAN_RESP_ID_MIN    0x7E8U
#define OBD2_CAN_RESP_ID_MAX    0x7EFU

// OBD-II Service Modes
#define OBD2_MODE_LIVE_DATA     0x01U
#define OBD2_MODE_READ_DTC      0x03U
#define OBD2_MODE_CLEAR_DTC     0x04U

// Common Mode 01 PIDs
#define OBD2_PID_COOLANT_TEMP   0x05U
#define OBD2_PID_ENGINE_RPM     0x0CU
#define OBD2_PID_VEHICLE_SPEED  0x0DU
#define OBD2_PID_THROTTLE_POS   0x11U

// Request builders (transmit over CAN1)
uint8_t OBD2_RequestPID(uint8_t pid);
uint8_t OBD2_RequestDTCs(void);
uint8_t OBD2_ClearDTCs(void);

// Response helpers
uint8_t OBD2_IsResponseFrame(const CAN_Frame_t *frame);
uint8_t OBD2_ParseRPM(const CAN_Frame_t *frame, uint16_t *rpm_out);
uint8_t OBD2_ParseSpeed(const CAN_Frame_t *frame, uint8_t *kmh_out);
uint8_t OBD2_ParseCoolantTemp(const CAN_Frame_t *frame, int16_t *temp_c_out);
uint8_t OBD2_ParseThrottle(const CAN_Frame_t *frame, uint8_t *pct_out);

// Single-frame DTC decoder (parses up to 2 DTCs into 6-byte null-terminated strings like "P0171")
void    OBD2_FormatDTC(uint8_t byte1, uint8_t byte2, char out_str[6]);
uint8_t OBD2_ParseDTCs(const CAN_Frame_t *frame, char dtc_list[2][6]);

#endif // OBD2_H