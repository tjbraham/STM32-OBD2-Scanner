#include "obd2.h"

uint8_t OBD2_RequestPID(uint8_t pid) {
    CAN_Frame_t req = {
        .id   = OBD2_CAN_REQ_ID,
        .dlc  = 8U,
        .data = {0x02U, OBD2_MODE_LIVE_DATA, pid, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U}
    };
    return CAN_Transmit(&req);
}

uint8_t OBD2_RequestDTCs(void) {
    CAN_Frame_t req = {
        .id   = OBD2_CAN_REQ_ID,
        .dlc  = 8U,
        .data = {0x01U, OBD2_MODE_READ_DTC, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U}
    };
    return CAN_Transmit(&req);
}

uint8_t OBD2_ClearDTCs(void) {
    CAN_Frame_t req = {
        .id   = OBD2_CAN_REQ_ID,
        .dlc  = 8U,
        .data = {0x01U, OBD2_MODE_CLEAR_DTC, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U}
    };
    return CAN_Transmit(&req);
}

uint8_t OBD2_IsResponseFrame(const CAN_Frame_t *frame) {
    return (frame->id >= OBD2_CAN_RESP_ID_MIN && frame->id <= OBD2_CAN_RESP_ID_MAX) ? 1U : 0U;
}

uint8_t OBD2_ParseRPM(const CAN_Frame_t *frame, uint16_t *rpm_out) {
    if (!OBD2_IsResponseFrame(frame)) {
        return 0U;
    }
    if (frame->data[1] != (OBD2_MODE_LIVE_DATA + 0x40U) || frame->data[2] != OBD2_PID_ENGINE_RPM) {
        return 0U;
    }

    uint16_t raw = ((uint16_t)frame->data[3] << 8) | frame->data[4];
    *rpm_out = raw >> 2; // Divide by 4
    return 1U;
}

uint8_t OBD2_ParseSpeed(const CAN_Frame_t *frame, uint8_t *kmh_out) {
    if (!OBD2_IsResponseFrame(frame)) {
        return 0U;
    }
    if (frame->data[1] != (OBD2_MODE_LIVE_DATA + 0x40U) || frame->data[2] != OBD2_PID_VEHICLE_SPEED) {
        return 0U;
    }

    *kmh_out = frame->data[3];
    return 1U;
}

uint8_t OBD2_ParseCoolantTemp(const CAN_Frame_t *frame, int16_t *temp_c_out) {
    if (!OBD2_IsResponseFrame(frame)) {
        return 0U;
    }
    if (frame->data[1] != (OBD2_MODE_LIVE_DATA + 0x40U) || frame->data[2] != OBD2_PID_COOLANT_TEMP) {
        return 0U;
    }

    *temp_c_out = (int16_t)frame->data[3] - 40;
    return 1U;
}

uint8_t OBD2_ParseThrottle(const CAN_Frame_t *frame, uint8_t *pct_out) {
    if (!OBD2_IsResponseFrame(frame)) {
        return 0U;
    }
    if (frame->data[1] != (OBD2_MODE_LIVE_DATA + 0x40U) || frame->data[2] != OBD2_PID_THROTTLE_POS) {
        return 0U;
    }

    *pct_out = (uint8_t)(((uint16_t)frame->data[3] * 100U) / 255U);
    return 1U;
}

void OBD2_FormatDTC(uint8_t byte1, uint8_t byte2, char out_str[6]) {
    static const char prefixes[]  = {'P', 'C', 'B', 'U'};
    static const char hex_chars[] = "0123456789ABCDEF";

    out_str[0] = prefixes[(byte1 >> 6) & 0x03U];
    out_str[1] = hex_chars[(byte1 >> 4) & 0x03U];
    out_str[2] = hex_chars[byte1 & 0x0FU];
    out_str[3] = hex_chars[(byte2 >> 4) & 0x0FU];
    out_str[4] = hex_chars[byte2 & 0x0FU];
    out_str[5] = '\0';
}

uint8_t OBD2_ParseDTCs(const CAN_Frame_t *frame, char dtc_list[2][6]) {
    if (!OBD2_IsResponseFrame(frame)) {
        return 0U;
    }
    // Single-frame Mode 03 response: upper nibble of data[0] must be 0x0 (Single Frame PCI)
    if ((frame->data[0] & 0xF0U) != 0x00U || frame->data[1] != (OBD2_MODE_READ_DTC + 0x40U)) {
        return 0U;
    }

    uint8_t dtc_count = frame->data[2];
    if (dtc_count > 2U) {
        dtc_count = 2U; // Clamp to 2 for single-frame parsing
    }

    uint8_t parsed = 0U;
    for (uint8_t i = 0U; i < dtc_count; i++) {
        uint8_t b1 = frame->data[3U + (i * 2U)];
        uint8_t b2 = frame->data[4U + (i * 2U)];

        // Skip empty 0x0000 padding slots
        if (b1 != 0x00U || b2 != 0x00U) {
            OBD2_FormatDTC(b1, b2, dtc_list[parsed]);
            parsed++;
        }
    }

    return parsed;
}