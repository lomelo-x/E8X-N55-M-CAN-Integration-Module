#include <FlexCAN_T4.h>
#include "can_core.h"
#include "gauge_sweep.h"

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can1;
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> can2;
CAN_message_t msg;

#define HEARTBEAT_LED 13

bool gaugeSweepDone = false;

void printCanMessageWithBus(const char* busName, const CAN_message_t &m) {
    Serial.print("[");
    Serial.print(busName);
    Serial.print("] 0x");
    Serial.print(m.id, HEX);
    Serial.print(" DLC:");
    Serial.print(m.len);
    Serial.print(" Data: ");
    for (int i = 0; i < m.len; i++) {
        if (m.buf[i] < 0x10) Serial.print("0");
        Serial.print(m.buf[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

bool checkIgnitionOn(const CAN_message_t &m, uint8_t &lastBuf1) {
    if (m.id == 0x480 && m.len == 8) {
        Serial.print("0x480 message buf[1]: 0x");
        Serial.print(m.buf[1], HEX);
        Serial.print(" lastBuf1: 0x");
        Serial.println(lastBuf1, HEX);

        // Trigger when buf[1] changes TO 0x42 (ignition ON)
        if (m.buf[1] == 0x42 && lastBuf1 != 0x42) {
            Serial.println("Ignition ON detected by transition!");
            lastBuf1 = m.buf[1];
            return true;
        }
        // Reset flag when buf[1] changes FROM 0x42 (ignition OFF)
        else if (m.buf[1] != 0x42 && lastBuf1 == 0x42) {
            Serial.println("Ignition OFF detected by transition!");
            gaugeSweepDone = false; // Reset flag to allow sweep on next ignition
        }
        lastBuf1 = m.buf[1];
    }
    return false;
}

// Filter for monitored CAN IDs (optional)
bool isMonitoredID(uint32_t id) {
    return (id == 0x480 || id == 0x130 || id == 0x12F);
}

void setup(void) {
    Serial.begin(115200);
    pinMode(HEARTBEAT_LED, OUTPUT);

    // Blink rapidly for 2 seconds during initial power-up
    for (int i = 0; i < 8; ++i) {
        digitalWrite(HEARTBEAT_LED, HIGH);
        delay(125);
        digitalWrite(HEARTBEAT_LED, LOW);
        delay(125);
    }

    Serial.println("E8X-M-CAN Initializing...");
    can1.begin();
    can1.setBaudRate(500000);
    can1.setTX(DEF);
    can1.setRX(DEF);
    can2.begin();
    can2.setBaudRate(500000);
    can2.setTX(DEF);
    can2.setRX(DEF);
    delay(100);

    Serial.println("CAN Bus Monitor Starting...");
    Serial.println("Waiting for CAN messages...");

    Serial.println("Gauge Sweep Initializing...");
    initializeGaugeMessages();
}

void loop() {
    static uint32_t lastMillis = 0;
    static uint8_t state = 0;
    uint32_t now = millis();

    // Track previous buf[1] for ignition transition detection
    static uint8_t last_480_buf1 = 0xFF;

    CAN_message_t rx_msg;

    // Read and process CAN1 messages
    while (can1.read(rx_msg)) {
        if (isMonitoredID(rx_msg.id)) {
            printCanMessageWithBus("CAN1", rx_msg);

            if (!gaugeSweepDone && checkIgnitionOn(rx_msg, last_480_buf1)) {
                Serial.println("Ignition detected on CAN1 — starting gauge sweep...");
                performGaugeSweep();
                gaugeSweepDone = true;
            }
        }
    }

    // Read and process CAN2 messages
    while (can2.read(rx_msg)) {
        if (isMonitoredID(rx_msg.id)) {
            printCanMessageWithBus("CAN2", rx_msg);

            if (!gaugeSweepDone && checkIgnitionOn(rx_msg, last_480_buf1)) {
                Serial.println("Ignition detected on CAN2 — starting gauge sweep...");
                performGaugeSweep();
                gaugeSweepDone = true;
            }
        }
    }

    // Heartbeat LED: rapid blink during sweep, double pulse otherwise
    if (rapid_blink) {
        // Rapid blink during gauge sweep
        digitalWrite(HEARTBEAT_LED, (now / 100) % 2); // Blink every 100ms
    } else {
        // Normal double pulse every ~2 seconds
        switch (state) {
            case 0: digitalWrite(HEARTBEAT_LED, HIGH);
                    if (now - lastMillis >= 150) { lastMillis = now; state = 1; }
                    break;
            case 1: digitalWrite(HEARTBEAT_LED, LOW);
                    if (now - lastMillis >= 150) { lastMillis = now; state = 2; }
                    break;
            case 2: digitalWrite(HEARTBEAT_LED, HIGH);
                    if (now - lastMillis >= 150) { lastMillis = now; state = 3; }
                    break;
            case 3: digitalWrite(HEARTBEAT_LED, LOW);
                    if (now - lastMillis >= 1500) { lastMillis = now; state = 0; }
                    break;
        }
    }
}