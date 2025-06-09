#ifndef CAN_CORE_H
#define CAN_CORE_H

#include <FlexCAN_T4.h>

// CAN bus configuration
#define KCAN_BAUD 100000  // 100kbps
#define PTCAN_BAUD 500000 // 500kbps
#define KCAN_RX_SIZE 1024
#define KCAN_TX_SIZE 128
#define PTCAN_RX_SIZE 1024
#define PTCAN_TX_SIZE 128

// Global CAN objects
extern FlexCAN_T4<CAN1, KCAN_RX_SIZE, KCAN_TX_SIZE> KCAN;
extern FlexCAN_T4<CAN2, PTCAN_RX_SIZE, PTCAN_TX_SIZE> PTCAN;
extern CAN_message_t k_msg;   // Global buffer for K-CAN received messages
extern CAN_message_t pt_msg;  // Global buffer for PT-CAN received messages

// CAN message helper functions
CAN_message_t make_msg_buf(uint32_t id, uint8_t len, uint8_t *data);
void kcan_write_msg(CAN_message_t &msg);
void ptcan_write_msg(CAN_message_t &msg);

// Initialize CAN communication
void initialize_can(void);

#endif // CAN_CORE_H 