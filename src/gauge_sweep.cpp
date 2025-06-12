#include "gauge_sweep.h"
#include <Arduino.h>
#include <string.h> // For memcpy

extern FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> can2;
extern CAN_message_t msg;
extern bool rapid_blink;

void initializeGaugeMessages() {
    Serial.println("Initializing gauge messages...");
    msg.len = 8;
    msg.flags.extended = 0;
    msg.flags.remote = 0;
    msg.flags.overrun = 0;
    msg.flags.reserved = 0;
    Serial.println("Gauge messages initialized");
}

void sendGaugeJob(uint8_t jobCode, uint8_t arg1, uint8_t arg2) {
    CAN_message_t sweep;
    sweep.id = 0x6F1;
    sweep.len = 8;
    sweep.flags.extended = 0;
    sweep.flags.remote = 0;
    sweep.flags.overrun = 0;
    sweep.flags.reserved = 0;

    // Format: 60 05 30 [jobCode] 06 [arg1] [arg2] 00
    sweep.buf[0] = 0x60;
    sweep.buf[1] = 0x05;
    sweep.buf[2] = 0x30;
    sweep.buf[3] = jobCode;
    sweep.buf[4] = 0x06;
    sweep.buf[5] = arg1;
    sweep.buf[6] = arg2;
    sweep.buf[7] = 0x00;

    if (!can2.write(sweep)) {
        Serial.print("Failed to send job 0x");
        Serial.println(jobCode, HEX);
    }
    delay(50); // slight delay between jobs
}

void performGaugeSweep() {
    Serial.println("Starting gauge sweep...");
    rapid_blink = true;

    // Speedo: jobCode 0x20, args 0x12 0x11
    sendGaugeJob(0x20, 0x12, 0x11);

    // Tach: jobCode 0x21, args 0x12 0x3D
    sendGaugeJob(0x21, 0x12, 0x3D);

    // Fuel: jobCode 0x22, args 0x07 0x4E
    sendGaugeJob(0x22, 0x07, 0x4E);

    // Oil: jobCode 0x23, args 0x07 0x12
    sendGaugeJob(0x23, 0x07, 0x12);

    rapid_blink = false;
    Serial.println("Gauge sweep completed");
}