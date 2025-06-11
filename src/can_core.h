#ifndef CAN_CORE_H
#define CAN_CORE_H

#include <Arduino.h>
#include <FlexCAN_T4.h>

// CAN bus configuration
#define KCAN_BAUD 100000  // 100kbps
#define PTCAN_BAUD 500000 // 500kbps
#define KCAN_RX_SIZE 1024
#define KCAN_TX_SIZE 128
#define PTCAN_RX_SIZE 1024
#define PTCAN_TX_SIZE 128

// Global CAN objects
extern FlexCAN_T4<CAN_DEV_TABLE::CAN1, RX_SIZE_256, TX_SIZE_16> KCAN;
extern FlexCAN_T4<CAN_DEV_TABLE::CAN2, RX_SIZE_256, TX_SIZE_16> PTCAN;
extern CAN_message_t k_msg;   // Global buffer for K-CAN received messages
extern CAN_message_t pt_msg;  // Global buffer for PT-CAN received messages

// Heartbeat rapid blink flag
extern bool rapid_blink;

// Function declarations
void kcan_receive_callback(const CAN_message_t &msg);
void ptcan_receive_callback(const CAN_message_t &msg);
void initializeGaugeMessages();
void performGaugeSweep();

#endif // CAN_CORE_H 