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

    Serial.print("Sending gauge job 0x");
    Serial.print(jobCode, HEX);
    Serial.print(" with args 0x");
    Serial.print(arg1, HEX);
    Serial.print(" 0x");
    Serial.println(arg2, HEX);

    if (!can2.write(sweep)) {
        Serial.print("Failed to send job 0x");
        Serial.println(jobCode, HEX);
    } else {
        Serial.println("CAN message sent successfully");
    }
    delay(50); // slight delay between jobs
}

void sendGaugeRelease(uint8_t jobCode) {
    CAN_message_t release;
    release.id = 0x6F1;
    release.len = 8;
    release.flags.extended = 0;
    release.flags.remote = 0;
    release.flags.overrun = 0;
    release.flags.reserved = 0;

    // Format: 60 03 30 [jobCode] 00 00 00 00
    release.buf[0] = 0x60;
    release.buf[1] = 0x03;
    release.buf[2] = 0x30;
    release.buf[3] = jobCode;
    release.buf[4] = 0x00;
    release.buf[5] = 0x00;
    release.buf[6] = 0x00;
    release.buf[7] = 0x00;

    Serial.print("Sending gauge release for 0x");
    Serial.println(jobCode, HEX);

    if (!can2.write(release)) {
        Serial.print("Failed to send release for job 0x");
        Serial.println(jobCode, HEX);
    } else {
        Serial.println("Release message sent successfully");
    }
    delay(50); // slight delay between jobs
}

void performGaugeSweep() {
    Serial.println("Starting gauge sweep...");
    rapid_blink = true;

    // Step 1: Sweep to maximum values
    Serial.println("Raising needles to maximum...");
    
    // Speedo: jobCode 0x20, args 0x12 0x11 (325 km/h)
    sendGaugeJob(0x20, 0x12, 0x11);
    delay(100);
    
    // Tach: jobCode 0x21, args 0x12 0x3D (8000 RPM)
    sendGaugeJob(0x21, 0x12, 0x3D);
    delay(100);
    
    // Fuel: jobCode 0x22, args 0x07 0x4E (100%)
    sendGaugeJob(0x22, 0x07, 0x4E);
    delay(100);
    
    // Oil: jobCode 0x23, args 0x07 0x12 (150°C)
    sendGaugeJob(0x23, 0x07, 0x12);
    delay(300);

    // Step 2: Hold at maximum
    Serial.println("Holding at maximum...");
    delay(500);

    // Step 3: Sweep to minimum values
    Serial.println("Lowering needles to minimum...");
    
    // Tach: jobCode 0x21, args 0x00 0x00 (0 RPM)
    sendGaugeJob(0x21, 0x00, 0x00);
    delay(100);
    
    // Speedo: jobCode 0x20, args 0x00 0x00 (0 km/h)
    sendGaugeJob(0x20, 0x00, 0x00);
    delay(100);
    
    // Oil: jobCode 0x23, args 0x00 0x00 (0°C)
    sendGaugeJob(0x23, 0x00, 0x00);
    delay(100);
    
    // Fuel: jobCode 0x22, args 0x00 0x00 (0%)
    sendGaugeJob(0x22, 0x00, 0x00);
    delay(300);

    // Step 4: Hold at minimum
    Serial.println("Holding at minimum...");
    delay(500);

    // Step 5: Release control back to cluster
    Serial.println("Releasing control back to cluster...");
    
    // Release commands use jobCode 0x03 instead of 0x05
    // Speedo release: 60 03 30 20 00 00 00 00
    sendGaugeRelease(0x20);
    delay(100);
    
    // Tach release: 60 03 30 21 00 00 00 00
    sendGaugeRelease(0x21);
    delay(100);
    
    // Fuel release: 60 03 30 22 00 00 00 00
    sendGaugeRelease(0x22);
    delay(100);
    
    // Oil release: 60 03 30 23 00 00 00 00
    sendGaugeRelease(0x23);
    delay(100);

    // Send release commands again to ensure proper operation
    delay(500);
    sendGaugeRelease(0x21);
    delay(100);
    sendGaugeRelease(0x20);
    delay(100);
    sendGaugeRelease(0x23);
    delay(100);
    sendGaugeRelease(0x22);

    rapid_blink = false;
    Serial.println("Gauge sweep completed");
}