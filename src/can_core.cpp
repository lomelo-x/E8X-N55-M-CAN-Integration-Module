#include "can_core.h"

bool rapid_blink = false;

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