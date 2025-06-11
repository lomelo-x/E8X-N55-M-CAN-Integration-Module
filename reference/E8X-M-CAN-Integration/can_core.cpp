#include "can_core.h"

// Global CAN objects
FlexCAN_T4<CAN1, KCAN_RX_SIZE, KCAN_TX_SIZE> KCAN;
FlexCAN_T4<CAN2, PTCAN_RX_SIZE, PTCAN_TX_SIZE> PTCAN;
CAN_message_t k_msg;
CAN_message_t pt_msg;

// CAN receive callbacks
void kcan_receive_callback(const CAN_message_t &msg) {
    k_msg = msg;  // Store in global buffer
}

void ptcan_receive_callback(const CAN_message_t &msg) {
    pt_msg = msg;  // Store in global buffer
}

void initialize_can(void) {
    // Initialize K-CAN (CAN1)
    KCAN.begin();
    KCAN.setBaudRate(KCAN_BAUD);
    KCAN.enableFIFO();
    KCAN.enableFIFOInterrupt();
    KCAN.onReceive(kcan_receive_callback);
    KCAN.mailboxStatus();

    // Initialize PT-CAN (CAN2)
    PTCAN.begin();
    PTCAN.setBaudRate(PTCAN_BAUD);
    PTCAN.enableFIFO();
    PTCAN.enableFIFOInterrupt();
    PTCAN.onReceive(ptcan_receive_callback);
    PTCAN.mailboxStatus();
}

// Create a CAN message buffer
CAN_message_t make_msg_buf(uint32_t id, uint8_t len, uint8_t *data) {
    CAN_message_t msg;
    msg.id = id;
    msg.len = len;
    for (uint8_t i = 0; i < len; i++) {
        msg.buf[i] = data[i];
    }
    return msg;
}

// Send a CAN message on K-CAN
void kcan_write_msg(CAN_message_t &msg) {
    KCAN.write(msg);
}

// Send a CAN message on PT-CAN
void ptcan_write_msg(CAN_message_t &msg) {
    PTCAN.write(msg);
}

void initializeGaugeMessages() {
    // Implementation of initializeGaugeMessages function
}

void performGaugeSweep() {
    // Implementation of performGaugeSweep function
} 