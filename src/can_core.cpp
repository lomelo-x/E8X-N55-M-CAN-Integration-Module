#include "can_core.h"

// Define global CAN objects
FlexCAN_T4<CAN_DEV_TABLE::CAN1, RX_SIZE_256, TX_SIZE_16> KCAN;
FlexCAN_T4<CAN_DEV_TABLE::CAN2, RX_SIZE_256, TX_SIZE_16> PTCAN;
CAN_message_t k_msg;   // Global buffer for K-CAN received messages
CAN_message_t pt_msg;  // Global buffer for PT-CAN received messages

// CAN message callback functions
void kcan_receive_callback(const CAN_message_t &msg) {
    // Handle K-CAN messages
    k_msg = msg;
}

void ptcan_receive_callback(const CAN_message_t &msg) {
    // Handle PT-CAN messages
    pt_msg = msg;
}

void initializeGaugeMessages() {
    // Initialize message structures for gauge sweep
    k_msg.len = 8;
    k_msg.flags.extended = 0;
    k_msg.flags.remote = 0;
    k_msg.flags.overrun = 0;
    k_msg.flags.reserved = 0;

    pt_msg.len = 8;
    pt_msg.flags.extended = 0;
    pt_msg.flags.remote = 0;
    pt_msg.flags.overrun = 0;
    pt_msg.flags.reserved = 0;
}

void performGaugeSweep() {
    // Perform gauge sweep animation
    for (uint8_t i = 0; i <= 100; i++) {
        // Update speedometer (0x1A6)
        k_msg.id = 0x1A6;
        k_msg.buf[0] = i; // Speed value
        KCAN.write(k_msg);

        // Update tachometer (0x0AA)
        k_msg.id = 0x0AA;
        k_msg.buf[0] = i; // RPM value
        KCAN.write(k_msg);

        // Update oil temperature (0x1A6)
        k_msg.id = 0x1A6;
        k_msg.buf[1] = i; // Temperature value
        KCAN.write(k_msg);

        delay(50); // Adjust speed of sweep
    }
} 