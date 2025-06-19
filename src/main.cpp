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
    
    Serial.println("System ready! Type 'ping' to test communication.");
    Serial.println("Waiting for ignition and engine start...");
    Serial.println("=== SETUP COMPLETED ===");
}

void loop() {
    static uint32_t lastMillis = 0;
    static uint8_t state = 0;
    static uint32_t lastTestTime = 0;
    static bool ignitionOn = false;
    static bool engineRunning = false;
    static bool sweepTriggered = false;
    uint32_t now = millis();

    // Process CAN messages for ignition detection
    CAN_message_t canMsg;
    
    // Check CAN1 (K-CAN) for ignition and engine messages
    if (can1.read(canMsg)) {
        // Debug: Print all CAN messages for troubleshooting
        Serial.print("CAN1 ID: 0x");
        Serial.print(canMsg.id, HEX);
        Serial.print(" Data: ");
        for (int i = 0; i < canMsg.len; i++) {
            if (canMsg.buf[i] < 0x10) Serial.print("0");
            Serial.print(canMsg.buf[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        
        switch (canMsg.id) {
            case 0x480: // Terminal status
                if (canMsg.len >= 1) {
                    // Correct ignition detection: bits 2-3 of buf[0]
                    // 0 = IGK_OFF, 1 = IGK_ON, 2 = INVALID_SIGNAL
                    uint8_t ignition_bits = (canMsg.buf[0] & 0b1100) >> 2;
                    bool newIgnitionState = (ignition_bits == 1);
                    
                    Serial.print("0x480 ignition bits: ");
                    Serial.print(ignition_bits);
                    Serial.print(" (");
                    Serial.print(newIgnitionState ? "ON" : "OFF");
                    Serial.println(")");
                    
                    if (newIgnitionState && !ignitionOn) {
                        Serial.println("Ignition ON detected!");
                        ignitionOn = true;
                        sweepTriggered = false; // Reset sweep flag for new ignition cycle
                    } else if (!newIgnitionState && ignitionOn) {
                        Serial.println("Ignition OFF detected!");
                        ignitionOn = false;
                        engineRunning = false;
                        sweepTriggered = false;
                    }
                }
                break;
                
            case 0x130: // Alternative terminal status
                if (canMsg.len >= 1) {
                    // Check for ignition in 0x130 message
                    uint8_t ignition_bits = (canMsg.buf[0] & 0b1100) >> 2;
                    bool newIgnitionState = (ignition_bits == 1);
                    
                    Serial.print("0x130 ignition bits: ");
                    Serial.print(ignition_bits);
                    Serial.print(" (");
                    Serial.print(newIgnitionState ? "ON" : "OFF");
                    Serial.println(")");
                    
                    if (newIgnitionState && !ignitionOn) {
                        Serial.println("Ignition ON detected from 0x130!");
                        ignitionOn = true;
                        sweepTriggered = false;
                    } else if (!newIgnitionState && ignitionOn) {
                        Serial.println("Ignition OFF detected from 0x130!");
                        ignitionOn = false;
                        engineRunning = false;
                        sweepTriggered = false;
                    }
                }
                break;
                
            case 0x3AF: // PDC bus states (might contain ignition info)
                Serial.println("0x3AF PDC bus states message received");
                break;
                
            case 0xAA: // Engine RPM and status
                if (canMsg.len >= 6) {
                    uint16_t rpm = ((canMsg.buf[2] << 8) | canMsg.buf[3]) * 4; // RPM calculation
                    bool newEngineState = (rpm > 200); // Engine running if RPM > 200
                    
                    Serial.print("0xAA RPM: ");
                    Serial.print(rpm);
                    Serial.print(" (");
                    Serial.print(newEngineState ? "RUNNING" : "STOPPED");
                    Serial.println(")");
                    
                    if (newEngineState && !engineRunning && ignitionOn && !sweepTriggered) {
                        Serial.println("Engine start detected! Triggering gauge sweep...");
                        engineRunning = true;
                        sweepTriggered = true;
                        testGaugeSweep();
                    } else if (!newEngineState && engineRunning) {
                        Serial.println("Engine stopped!");
                        engineRunning = false;
                    }
                }
                break;
        }
    }
    
    // Check CAN2 (PT-CAN) for additional engine messages
    if (can2.read(canMsg)) {
        // Add any PT-CAN message processing here if needed
    }

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
            Serial.print("Ignition: ");
            Serial.println(ignitionOn ? "ON" : "OFF");
            Serial.print("Engine: ");
            Serial.println(engineRunning ? "RUNNING" : "STOPPED");
            Serial.print("Sweep triggered: ");
            Serial.println(sweepTriggered ? "YES" : "NO");
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
        } else if (cmd == "trigger") {
            Serial.println("Manually triggering gauge sweep...");
            sweepTriggered = false; // Reset flag to allow manual trigger
            testGaugeSweep();
            gaugeSweepDone = true;
        } else if (cmd == "debug") {
            Serial.println("Enabling CAN message debug output...");
            // This will show all CAN messages in the main loop
        } else {
            Serial.println("Unknown command. Try: ping, status, restart, test, sweep, trigger, debug");
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