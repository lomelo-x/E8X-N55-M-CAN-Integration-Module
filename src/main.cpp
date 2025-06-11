#include <FlexCAN_T4.h>

#include "can_core.h"

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can1;
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> can2;
CAN_message_t msg;

#define HEARTBEAT_LED 13

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000);  // Wait for serial or timeout
    Serial.println("E8X N55 M-CAN Integration Module Starting...");

    // Configure K-CAN
    KCAN.begin();
    KCAN.setBaudRate(100000);  // 100kbps
    KCAN.setTX(22);            // K-CAN TX pin
    KCAN.setRX(23);            // K-CAN RX pin
    KCAN.enableFIFO();
    KCAN.enableFIFOInterrupt();

    // Configure PT-CAN
    PTCAN.begin();
    PTCAN.setBaudRate(500000);  // 500kbps
    PTCAN.setTX(0);             // PT-CAN TX pin
    PTCAN.setRX(1);             // PT-CAN RX pin
    PTCAN.enableFIFO();
    PTCAN.enableFIFOInterrupt();

    // Initialize gauge messages
    Serial.println("Gauge Sweep Initializing...");
    initializeGaugeMessages();

    Serial.println("CAN buses initialized");

    // Perform initial gauge sweep
    Serial.println("Gauge Sweep Starting...");
    performGaugeSweep();

    Serial.println("Gauge Sweep Completed");
    pinMode(HEARTBEAT_LED, OUTPUT);
}

void loop() {
    CAN_message_t msg;

    // Check K-CAN messages
    if (KCAN.read(msg)) {
        Serial.print("K-CAN ID: 0x");
        Serial.println(msg.id, HEX);
    }

    // Check PT-CAN messages
    if (PTCAN.read(msg)) {
        Serial.print("PT-CAN ID: 0x");
        Serial.println(msg.id, HEX);
    }
    static uint32_t lastMillis = 0;
    static uint8_t state = 0;
    static bool ledState = false;
    uint32_t now = millis();

    // Heartbeat LED logic (double pulse or rapid blink)
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
            case 0:
                digitalWrite(HEARTBEAT_LED, HIGH);
                if (now - lastMillis >= 150) {
                    lastMillis = now;
                    state = 1;
                }
                break;
            case 1:
                digitalWrite(HEARTBEAT_LED, LOW);
                if (now - lastMillis >= 150) {
                    lastMillis = now;
                    state = 2;
                }
                break;
            case 2:
                digitalWrite(HEARTBEAT_LED, HIGH);
                if (now - lastMillis >= 150) {
                    lastMillis = now;
                    state = 3;
                }
                break;
            case 3:
                digitalWrite(HEARTBEAT_LED, LOW);
                if (now - lastMillis >= 1500) {
                    lastMillis = now;
                    state = 0;
                }
                break;
        }
    }
}
