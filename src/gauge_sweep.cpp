#include "gauge_sweep.h"

#include <Arduino.h>

// Extern CAN objects and buffers
extern FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> can2;
extern CAN_message_t msg;
extern bool rapid_blink;

void initializeGaugeMessages() {
    // Initialize message structures for gauge sweep
    msg.len = 8;
    msg.flags.extended = 0;
    msg.flags.remote = 0;
    msg.flags.overrun = 0;
    msg.flags.reserved = 0;
}

// void performGaugeSweep() {
//     rapid_blink = true;
//     for (uint8_t i = 0; i <= 100; i++) {
//         // Update speedometer (0x1A6)
//         msg.id = 0x1A6;
//         msg.buf[0] = i;  // Speed value
//         can2.write(msg);

//         // Update tachometer (0x0AA)
//         msg.id = 0x0AA;
//         msg.buf[0] = i;  // RPM value
//         can2.write(msg);

//         // Update oil temperature (0x1A6)
//         msg.id = 0x1A6;
//         msg.buf[1] = i;  // Temperature value
//         can2.write(msg);

//         delay(50);  // Adjust speed of sweep
//     }
//     rapid_blink = false;
// }

void performGaugeSweep() {
    Serial.println("Gauge sweep: rapid_blink ON");
    rapid_blink = true;
    for (uint8_t i = 0; i <= 100; i++) {
        Serial.print("Sweep step: "); Serial.println(i);
        msg.id = 0x1A6;
        msg.buf[0] = i;
        Serial.println("Writing speedo...");
        can2.write(msg);

        msg.id = 0x0AA;
        msg.buf[0] = i;
        Serial.println("Writing tacho...");
        can2.write(msg);

        msg.id = 0x1A6;
        msg.buf[1] = i;
        Serial.println("Writing oil...");
        can2.write(msg);

        delay(50);
    }
    rapid_blink = false;
    Serial.println("Gauge sweep: rapid_blink OFF");
    Serial.println("Gauge Sweep Completed");
}
