#include "gauge_sweep.h"

#include <Arduino.h>

// Extern CAN objects and buffers
extern FlexCAN_T4<CAN_DEV_TABLE::CAN1, RX_SIZE_256, TX_SIZE_16> KCAN;
extern CAN_message_t k_msg;
extern bool rapid_blink;

void initializeGaugeMessages() {
    // Initialize message structures for gauge sweep
    k_msg.len = 8;
    k_msg.flags.extended = 0;
    k_msg.flags.remote = 0;
    k_msg.flags.overrun = 0;
    k_msg.flags.reserved = 0;
}

void performGaugeSweep() {
    rapid_blink = true;
    for (uint8_t i = 0; i <= 100; i++) {
        // Update speedometer (0x1A6)
        k_msg.id = 0x1A6;
        k_msg.buf[0] = i;  // Speed value
        KCAN.write(k_msg);

        // Update tachometer (0x0AA)
        k_msg.id = 0x0AA;
        k_msg.buf[0] = i;  // RPM value
        KCAN.write(k_msg);

        // Update oil temperature (0x1A6)
        k_msg.id = 0x1A6;
        k_msg.buf[1] = i;  // Temperature value
        KCAN.write(k_msg);

        delay(50);  // Adjust speed of sweep
    }
    rapid_blink = false;
}