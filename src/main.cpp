#include <FlexCAN_T4.h>
#include "can_core.h"
#include "gauge_sweep.h"

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can1;
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> can2;
CAN_message_t msg;

#define HEARTBEAT_LED 13
#define RESET_PIN 12

bool gaugeSweepDone = false;

// Global variables for engine detection
bool engineRunning = false;
uint16_t lastRPM = 0;
uint32_t lastRPMTime = 0;

// Simple gauge sweep function wrapper
void testGaugeSweep() {
    Serial.println("=== testGaugeSweep() STARTING ===");
    performGaugeSweep();
    Serial.println("=== testGaugeSweep() COMPLETED ===");
}

void setup(void) {
    Serial.begin(115200);
    Serial.println("=== SETUP STARTING ===");
    
    pinMode(HEARTBEAT_LED, OUTPUT);
    Serial.println("LED pin configured");
    
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, HIGH);
    Serial.println("Reset pin configured");

    // Blink rapidly for 2 seconds during initial power-up
    Serial.println("Starting LED blink sequence...");
    for (int i = 0; i < 8; ++i) {
        digitalWrite(HEARTBEAT_LED, HIGH);
        delay(125);
        digitalWrite(HEARTBEAT_LED, LOW);
        delay(125);
        Serial.print("Blink ");
        Serial.println(i + 1);
    }
    Serial.println("LED blink sequence completed");

    Serial.println("E8X-M-CAN Initializing...");
    
    // Test CAN initialization
    Serial.println("Initializing CAN1...");
    can1.begin();
    can1.setBaudRate(100000); // 100kbps for K-CAN
    can1.setTX(22); // K-CAN TX pin
    can1.setRX(23); // K-CAN RX pin
    can1.enableFIFO();
    Serial.println("CAN1 initialized");
    
    Serial.println("Initializing CAN2...");
    can2.begin();
    can2.setBaudRate(500000); // 500kbps for PT-CAN
    can2.setTX(0); // PT-CAN TX pin
    can2.setRX(1); // PT-CAN RX pin
    can2.enableFIFO();
    Serial.println("CAN2 initialized");
    
    // Initialize gauge sweep functionality
    Serial.println("Initializing gauge sweep...");
    initializeGaugeMessages();
    Serial.println("Gauge sweep initialized");
    
    Serial.println("Testing gauge sweep on startup...");
    
    // Run gauge sweep automatically on startup
    Serial.println("About to call testGaugeSweep()...");
    testGaugeSweep();
    Serial.println("testGaugeSweep() completed");
    
    Serial.println("System ready! Type 'ping' to test communication.");
    Serial.println("=== SETUP COMPLETED ===");
}

void loop() {
    static uint32_t lastMillis = 0;
    static uint8_t state = 0;
    static uint32_t lastTestTime = 0;
    uint32_t now = millis();

    // Automatic test every 10 seconds
    if (now - lastTestTime > 10000) {
        Serial.println("Auto-test: System is running normally");
        lastTestTime = now;
    }

    // Check for serial commands (simplified)
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        Serial.print("Received command: '");
        Serial.print(cmd);
        Serial.println("'");
        
        if (cmd == "ping") {
            Serial.println("Pong! Teensy is alive.");
        } else if (cmd == "status") {
            Serial.print("gaugeSweepDone: ");
            Serial.println(gaugeSweepDone ? "true" : "false");
        } else if (cmd == "restart") {
            Serial.println("Restarting Teensy...");
            delay(1000);
            SCB_AIRCR = 0x05FA0004;
        } else if (cmd == "test") {
            Serial.println("Test function called successfully!");
            rapid_blink = true;
            delay(1000);
            rapid_blink = false;
            Serial.println("Test completed!");
        } else if (cmd == "sweep") {
            Serial.println("Testing gauge sweep...");
            testGaugeSweep();
            gaugeSweepDone = true;
        } else {
            Serial.println("Unknown command. Try: ping, status, restart, test, sweep");
        }
    }

    // Heartbeat LED: rapid blink during sweep, double pulse otherwise
    if (rapid_blink) {
        // Rapid blink during test
        digitalWrite(HEARTBEAT_LED, (now / 100) % 2);
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
    
    // Simple delay
    delay(50);
}