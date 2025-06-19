#include <FlexCAN_T4.h>
#include "can_core.h"

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

// Simple gauge sweep function
void testGaugeSweep() {
    Serial.println("=== testGaugeSweep() STARTING ===");
    Serial.println("Starting gauge sweep...");
    
    rapid_blink = true;
    Serial.println("Rapid blink enabled");

    // Step 1: Raise needles to maximum
    Serial.println("Raising needles to maximum...");
    
    // Speedo to max (0x20)
    CAN_message_t speedo_max;
    speedo_max.id = 0x6F1;
    speedo_max.len = 8;
    speedo_max.buf[0] = 0x60; speedo_max.buf[1] = 0x05; speedo_max.buf[2] = 0x30;
    speedo_max.buf[3] = 0x20; speedo_max.buf[4] = 0x06; speedo_max.buf[5] = 0x12;
    speedo_max.buf[6] = 0x11; speedo_max.buf[7] = 0x00;
    can2.write(speedo_max);
    delay(100);
    
    // Tach to max (0x21)
    CAN_message_t tach_max;
    tach_max.id = 0x6F1;
    tach_max.len = 8;
    tach_max.buf[0] = 0x60; tach_max.buf[1] = 0x05; tach_max.buf[2] = 0x30;
    tach_max.buf[3] = 0x21; tach_max.buf[4] = 0x06; tach_max.buf[5] = 0x12;
    tach_max.buf[6] = 0x3D; tach_max.buf[7] = 0x00;
    can2.write(tach_max);
    delay(100);
    
    // Oil to max (0x23)
    CAN_message_t oil_max;
    oil_max.id = 0x6F1;
    oil_max.len = 8;
    oil_max.buf[0] = 0x60; oil_max.buf[1] = 0x05; oil_max.buf[2] = 0x30;
    oil_max.buf[3] = 0x23; oil_max.buf[4] = 0x06; oil_max.buf[5] = 0x07;
    oil_max.buf[6] = 0x12; oil_max.buf[7] = 0x00;
    can2.write(oil_max);
    delay(300);
    
    Serial.println("Needles raised to maximum");
    
    // Step 2: Hold at maximum
    Serial.println("Holding at maximum...");
    delay(500);
    
    // Step 3: Lower needles to minimum
    Serial.println("Lowering needles to minimum...");
    
    // Tach to min (0x21)
    CAN_message_t tach_min;
    tach_min.id = 0x6F1;
    tach_min.len = 8;
    tach_min.buf[0] = 0x60; tach_min.buf[1] = 0x05; tach_min.buf[2] = 0x30;
    tach_min.buf[3] = 0x21; tach_min.buf[4] = 0x06; tach_min.buf[5] = 0x00;
    tach_min.buf[6] = 0x00; tach_min.buf[7] = 0x00;
    can2.write(tach_min);
    delay(100);
    
    // Speedo to min (0x20)
    CAN_message_t speedo_min;
    speedo_min.id = 0x6F1;
    speedo_min.len = 8;
    speedo_min.buf[0] = 0x60; speedo_min.buf[1] = 0x05; speedo_min.buf[2] = 0x30;
    speedo_min.buf[3] = 0x20; speedo_min.buf[4] = 0x06; speedo_min.buf[5] = 0x00;
    speedo_min.buf[6] = 0x00; speedo_min.buf[7] = 0x00;
    can2.write(speedo_min);
    delay(100);
    
    // Oil to min (0x23)
    CAN_message_t oil_min;
    oil_min.id = 0x6F1;
    oil_min.len = 8;
    oil_min.buf[0] = 0x60; oil_min.buf[1] = 0x05; oil_min.buf[2] = 0x30;
    oil_min.buf[3] = 0x23; oil_min.buf[4] = 0x06; oil_min.buf[5] = 0x00;
    oil_min.buf[6] = 0x00; oil_min.buf[7] = 0x00;
    can2.write(oil_min);
    delay(300);
    
    Serial.println("Needles lowered to minimum");
    
    // Step 4: Hold at minimum
    Serial.println("Holding at minimum...");
    delay(500);
    
    // Step 5: Release control back to cluster
    Serial.println("Releasing control back to cluster...");
    
    // Release speedo (0x20)
    CAN_message_t speedo_release;
    speedo_release.id = 0x6F1;
    speedo_release.len = 8;
    speedo_release.buf[0] = 0x60; speedo_release.buf[1] = 0x03; speedo_release.buf[2] = 0x30;
    speedo_release.buf[3] = 0x20; speedo_release.buf[4] = 0x00; speedo_release.buf[5] = 0x00;
    speedo_release.buf[6] = 0x00; speedo_release.buf[7] = 0x00;
    can2.write(speedo_release);
    delay(100);
    
    // Release tach (0x21)
    CAN_message_t tach_release;
    tach_release.id = 0x6F1;
    tach_release.len = 8;
    tach_release.buf[0] = 0x60; tach_release.buf[1] = 0x03; tach_release.buf[2] = 0x30;
    tach_release.buf[3] = 0x21; tach_release.buf[4] = 0x00; tach_release.buf[5] = 0x00;
    tach_release.buf[6] = 0x00; tach_release.buf[7] = 0x00;
    can2.write(tach_release);
    delay(100);
    
    // Release oil (0x23)
    CAN_message_t oil_release;
    oil_release.id = 0x6F1;
    oil_release.len = 8;
    oil_release.buf[0] = 0x60; oil_release.buf[1] = 0x03; oil_release.buf[2] = 0x30;
    oil_release.buf[3] = 0x23; oil_release.buf[4] = 0x00; oil_release.buf[5] = 0x00;
    oil_release.buf[6] = 0x00; oil_release.buf[7] = 0x00;
    can2.write(oil_release);
    delay(100);
    
    Serial.println("Control released back to cluster");
    
    rapid_blink = false;
    Serial.println("Rapid blink disabled");
    Serial.println("Gauge sweep completed");
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
    Serial.println("CAN1 initialized");
    
    Serial.println("Initializing CAN2...");
    can2.begin();
    Serial.println("CAN2 initialized");
    
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