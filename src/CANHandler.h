#ifndef CAN_HANDLER_H
#define CAN_HANDLER_H

#include <Arduino.h>
#include <FlexCAN_T4.h>

// We need two CAN buses:
// 1. K-CAN at 100kbps
// 2. PT-CAN at 500kbps
class CANHandler {
private:
    FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> ptCan; // PT-CAN on CAN1
    FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> kCan;  // K-CAN on CAN2

public:
    bool begin() {
        // Initialize PT-CAN (500kbps)
        ptCan.begin();
        ptCan.setBaudRate(500000);
        ptCan.setMaxMB(16); // Use 16 message buffers
        ptCan.enableFIFO();
        ptCan.enableFIFOInterrupt();
        
        // Initialize K-CAN (100kbps)
        kCan.begin();
        kCan.setBaudRate(100000);
        kCan.setMaxMB(16);
        kCan.enableFIFO();
        kCan.enableFIFOInterrupt();
        
        return true;
    }

    // PT-CAN methods (500kbps)
    bool writePTCAN(uint32_t id, uint8_t* data, uint8_t len) {
        CAN_message_t msg;
        msg.id = id;
        msg.len = len;
        memcpy(msg.buf, data, len);
        return ptCan.write(msg);
    }

    bool readPTCAN(uint32_t& id, uint8_t* data, uint8_t& len) {
        CAN_message_t msg;
        if (ptCan.read(msg)) {
            id = msg.id;
            len = msg.len;
            memcpy(data, msg.buf, len);
            return true;
        }
        return false;
    }

    // K-CAN methods (100kbps)
    bool writeKCAN(uint32_t id, uint8_t* data, uint8_t len) {
        CAN_message_t msg;
        msg.id = id;
        msg.len = len;
        memcpy(msg.buf, data, len);
        return kCan.write(msg);
    }

    bool readKCAN(uint32_t& id, uint8_t* data, uint8_t& len) {
        CAN_message_t msg;
        if (kCan.read(msg)) {
            id = msg.id;
            len = msg.len;
            memcpy(data, msg.buf, len);
            return true;
        }
        return false;
    }
};

#endif 