#include "can_core.h"

// Global CAN objects
FlexCAN_T4<CAN1, KCAN_RX_SIZE, KCAN_TX_SIZE> KCAN;
CAN_message_t k_msg;

void initialize_can(void) {
    // Initialize KCAN (CAN1)
    KCAN.begin();
    KCAN.setBaudRate(KCAN_BAUD);
    KCAN.enableFIFO();
    KCAN.enableFIFOInterrupt();
    KCAN.onReceive(can_receive_callback);
    KCAN.mailboxStatus();
}

// CAN receive callback
void can_receive_callback(const CAN_message_t &msg) {
    k_msg = msg;  // Store in global buffer
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

// Send a CAN message
void kcan_write_msg(CAN_message_t &msg) {
    KCAN.write(msg);
} 