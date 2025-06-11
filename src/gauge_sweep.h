#ifndef GAUGE_SWEEP_H
#define GAUGE_SWEEP_H

#include <FlexCAN_T4.h>

// Extern for rapid_blink flag
extern bool rapid_blink;

// Function declarations
void initializeGaugeMessages();
void performGaugeSweep();

#endif  // GAUGE_SWEEP_H
