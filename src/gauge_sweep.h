#ifndef GAUGE_SWEEP_H
#define GAUGE_SWEEP_H

#include <FlexCAN_T4.h>

// Extern for CAN objects and flags
extern FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> can2;
extern CAN_message_t msg;
extern bool rapid_blink;

// Function declarations
void initializeGaugeMessages();
void performGaugeSweep();

#endif  // GAUGE_SWEEP_H
