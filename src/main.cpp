#include <FlexCAN_T4.h>
#include "can_core.h"
#include "gauge_sweep.h"

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can1;
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> can2;
CAN_message_t msg;

#define HEARTBEAT_LED 13

void setup(void) {
    Serial.begin(115200);
    pinMode(HEARTBEAT_LED, OUTPUT);

    // Blink rapidly for 2 seconds during initial power-up
    for (int i = 0; i < 8; ++i) { // 8 blinks at 250ms = 2 seconds
        digitalWrite(HEARTBEAT_LED, HIGH);
        delay(125);
        digitalWrite(HEARTBEAT_LED, LOW);
        delay(125);
    }

    Serial.println("E8X-M-CAN Initializing...");
    can1.begin();
    can1.setBaudRate(500000);  // PT-CAN is actually on CAN1
    can1.setTX(DEF);           // Use default TX pin for PTCAN
    can1.setRX(DEF);           // Use default RX pin for PTCAN
    can2.begin();
    can2.setBaudRate(100000);  // K-CAN is actually on CAN2
    can2.setTX(DEF);           // Use default TX pin for KCAN
    can2.setRX(DEF);           // Use default RX pin for KCAN
    delay(100);
    Serial.println("CAN Bus Monitor Starting...");
    Serial.println("Waiting for CAN messages...");

    Serial.println("Gauge Sweep Initializing...");
    initializeGaugeMessages();
    Serial.println("Gauge Sweep Starting...");
    // performGaugeSweep();
    Serial.println("Gauge Sweep Completed");
}

void loop() {
    static uint32_t lastMillis = 0;
    static uint8_t state = 0;
    static bool ledState = false;
    uint32_t now = millis();

    // Check for CAN messages on both buses
    CAN_message_t rx_msg;
    
    // Check CAN1 (K-CAN)
    while (can1.read(rx_msg)) {
        Serial.print("K-CAN ID: 0x");
        Serial.print(rx_msg.id, HEX);
        Serial.print(" Data: ");
        for (int i = 0; i < rx_msg.len; i++) {
            Serial.print(rx_msg.buf[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }

    // Check CAN2 (PT-CAN)
    while (can2.read(rx_msg)) {
        Serial.print("PT-CAN ID: 0x");
        Serial.print(rx_msg.id, HEX);
        Serial.print(" Data: ");
        for (int i = 0; i < rx_msg.len; i++) {
            Serial.print(rx_msg.buf[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }

    // Heartbeat LED logic
    if (rapid_blink) {
        // Rapid blink: 50ms on, 50ms off
        if (now - lastMillis >= 50) {
            ledState = !ledState;
            digitalWrite(HEARTBEAT_LED, ledState ? HIGH : LOW);
            lastMillis = now;
        }
    } else {
        // Double pulse heartbeat
        switch (state) {
            case 0: digitalWrite(HEARTBEAT_LED, HIGH);
                    if (now - lastMillis >= 150) { lastMillis = now; state = 1; } break;
            case 1: digitalWrite(HEARTBEAT_LED, LOW);
                    if (now - lastMillis >= 150) { lastMillis = now; state = 2; } break;
            case 2: digitalWrite(HEARTBEAT_LED, HIGH);
                    if (now - lastMillis >= 150) { lastMillis = now; state = 3; } break;
            case 3: digitalWrite(HEARTBEAT_LED, LOW);
                    if (now - lastMillis >= 1500) { lastMillis = now; state = 0; } break;
        }
    }
}

