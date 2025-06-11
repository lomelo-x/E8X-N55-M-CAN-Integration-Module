#if !defined(ARDUINO) 
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#endif

#include <Arduino.h>
#include <FlexCAN_T4.h>
#include "imxrt_flexcan.h"
#include <stdint.h>

// CAN bus definitions
// Note: Actual Teensy connections:
// K-CAN:  RX on pin 0, TX on pin 1
// PT-CAN: RX on pin 23, TX on pin 22
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> KCAN; // K-CAN (100kbps) - pins 0(RX)/1(TX)
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> PTCAN; // PT-CAN (500kbps) - pins 23(RX)/22(TX)

// Test message configuration
const uint32_t TEST_MSG_ID = 0x123;  // Test message ID
const uint32_t TEST_INTERVAL = 1000; // Send test message every 1 second
uint32_t lastTestTime = 0;
uint32_t kcanSentCount = 0;
uint32_t kcanReceivedCount = 0;
uint32_t ptcanSentCount = 0;
uint32_t ptcanReceivedCount = 0;

// Baud rate selector for easy testing
#define CAN_BAUD_RATE 125000  // Change to 250000 or 500000 to test different rates

void sendTestMessage() {
    CAN_message_t testMsg;
    testMsg.id = TEST_MSG_ID;
    testMsg.len = 8;
    testMsg.buf[0] = 0xAA;  // Test pattern
    testMsg.buf[1] = 0x55;
    testMsg.buf[2] = 0x12;
    testMsg.buf[3] = 0x34;
    testMsg.buf[4] = 0x56;
    testMsg.buf[5] = 0x78;
    testMsg.buf[6] = 0x9A;
    testMsg.buf[7] = 0xBC;

    // Send on K-CAN
    if (KCAN.write(testMsg)) {
        kcanSentCount++;
        Serial.print("K-CAN: Sent message #");
        Serial.print(kcanSentCount);
        Serial.print(" (ID: 0x");
        Serial.print(testMsg.id, HEX);
        Serial.println(")");
    } else {
        Serial.println("K-CAN: Failed to send test message");
    }
    
    // Send on PT-CAN
    if (PTCAN.write(testMsg)) {
        ptcanSentCount++;
        Serial.print("PT-CAN: Sent message #");
        Serial.print(ptcanSentCount);
        Serial.print(" (ID: 0x");
        Serial.print(testMsg.id, HEX);
        Serial.println(")");
    } else {
        Serial.println("PT-CAN: Failed to send test message");
    }
}

void printStats() {
    Serial.println("\n=== CAN Bus Statistics ===");
    Serial.print("K-CAN:  Sent: ");
    Serial.print(kcanSentCount);
    Serial.print("  Received: ");
    Serial.println(kcanReceivedCount);
    Serial.print("PT-CAN: Sent: ");
    Serial.print(ptcanSentCount);
    Serial.print("  Received: ");
    Serial.println(ptcanReceivedCount);
    Serial.println("=======================\n");
}

void printCANMessage(const char* bus, const CAN_message_t &msg) {
    if (strcmp(bus, "K-CAN") == 0) {
        kcanReceivedCount++;
    } else if (strcmp(bus, "PT-CAN") == 0) {
        ptcanReceivedCount++;
    }
    
    Serial.print(bus);
    Serial.print(": Received message (ID: 0x");
    Serial.print(msg.id, HEX);
    Serial.print(") LEN: ");
    Serial.print(msg.len);
    Serial.print(" DATA: ");
    for (int i = 0; i < msg.len; i++) {
        if (msg.buf[i] < 0x10) Serial.print("0");
        Serial.print(msg.buf[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
    
    // Wait for Serial to be ready - needed for USB Serial on Teensy
    while (!Serial) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // Blink LED while waiting
        delay(100);
    }
    digitalWrite(LED_BUILTIN, HIGH);  // LED on once Serial is ready
    
    Serial.println("\n=== DUAL CAN LOGGER ===");
    Serial.print("K-CAN: pins 0(RX)/1(TX) @ ");
    Serial.print(CAN_BAUD_RATE);
    Serial.println("bps");
    Serial.print("PT-CAN: pins 23(RX)/22(TX) @ ");
    Serial.print(CAN_BAUD_RATE);
    Serial.println("bps");
    Serial.println("Sending test messages every 1 second");
    Serial.println("Test message ID: 0x123");
    Serial.println("=======================");

    // Initialize K-CAN (CAN1)
    KCAN.begin();
    KCAN.setBaudRate(CAN_BAUD_RATE);
    KCAN.setRX(DEF); // pin 0 (RX)
    KCAN.setTX(DEF); // pin 1 (TX)
    KCAN.setMaxMB(16);
    KCAN.enableFIFO();
    KCAN.enableFIFOInterrupt();

    // Initialize PT-CAN (CAN2)
    PTCAN.begin();
    PTCAN.setBaudRate(CAN_BAUD_RATE);
    PTCAN.setRX(ALT); // pin 23 (RX)
    PTCAN.setTX(ALT); // pin 22 (TX)
    PTCAN.setMaxMB(16);
    PTCAN.enableFIFO();
    PTCAN.enableFIFOInterrupt();

    Serial.println("Ready to log both CAN buses.");
    digitalWrite(LED_BUILTIN, LOW);  // Ready to start normal operation
}

void loop() {
    CAN_message_t msg;
    uint32_t currentTime = millis();
    static uint32_t lastErrorPrint = 0;

    // Send test message every TEST_INTERVAL milliseconds
    if (currentTime - lastTestTime >= TEST_INTERVAL) {
        sendTestMessage();
        printStats();  // Print statistics after each test message
        lastTestTime = currentTime;
    }

    // Read K-CAN
    while (KCAN.read(msg)) {
        printCANMessage("K-CAN", msg);
    }
    
    // Read PT-CAN
    while (PTCAN.read(msg)) {
        printCANMessage("PT-CAN", msg);
    }

    // Print CAN error counts and status every second
    if (currentTime - lastErrorPrint >= 1000) {
        Serial.print("KCAN error count: ");
        Serial.println(KCAN.getErrorCount());
        Serial.print("PTCAN error count: ");
        Serial.println(PTCAN.getErrorCount());
        Serial.print("KCAN status: 0x");
        Serial.println(KCAN.getBusStatus(), HEX);
        Serial.print("PTCAN status: 0x");
        Serial.println(PTCAN.getBusStatus(), HEX);
        lastErrorPrint = currentTime;
    }

    // Heartbeat LED pattern (non-blocking)
    static uint32_t lastTime = 0;
    static uint8_t state = 0;
    uint32_t now = millis();

    switch (state) {
        case 0:
            digitalWrite(LED_BUILTIN, HIGH);
            lastTime = now;
            state = 1;
            break;
        case 1:
            if (now - lastTime >= 100) { // ON for 100ms
                digitalWrite(LED_BUILTIN, LOW);
                lastTime = now;
                state = 2;
            }
            break;
        case 2:
            if (now - lastTime >= 100) { // OFF for 100ms
                digitalWrite(LED_BUILTIN, HIGH);
                lastTime = now;
                state = 3;
            }
            break;
        case 3:
            if (now - lastTime >= 100) { // ON for 100ms
                digitalWrite(LED_BUILTIN, LOW);
                lastTime = now;
                state = 4;
            }
            break;
        case 4:
            if (now - lastTime >= 700) { // OFF for 700ms pause before repeat
                state = 0;
            }
            break;
    }
} 