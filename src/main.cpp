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
    can1.setBaudRate(125000);  // Set to your K-CAN baud rate if needed
    can1.setTX(DEF);           // Use default TX pin for KCAN
    can1.setRX(DEF);           // Use default RX pin for KCAN
    can2.begin();
    can2.setBaudRate(125000);  // Set to your PT-CAN baud rate if needed
    can2.setTX(DEF);           // Use default TX pin for PTCAN
    can2.setRX(DEF);           // Use default RX pin for PTCAN
    delay(100);
    Serial.println("Gauge Sweep Initializing...");
    initializeGaugeMessages();
    Serial.println("Gauge Sweep Starting...");
    performGaugeSweep();
    Serial.println("Gauge Sweep Completed");
}

void loop() {
    static uint32_t lastMillis = 0;
    static uint8_t state = 0;
    static bool ledState = false;
    uint32_t now = millis();

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
