#include <Arduino.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

#include <FlexCAN_T4.h>
#include <imxrt_flexcan.h>
#include <stdint.h>

// CAN bus definitions using the correct namespace
FlexCAN_T4<CAN_DEV_TABLE::CAN1, RX_SIZE_256, TX_SIZE_16> KCAN; // K-CAN (100kbps)
FlexCAN_T4<CAN_DEV_TABLE::CAN2, RX_SIZE_256, TX_SIZE_16> PTCAN; // PT-CAN (500kbps)

// Message buffers for gauge control
CAN_message_t speedo_needle_max_buf, speedo_needle_min_buf;
CAN_message_t tacho_needle_max_buf, tacho_needle_min_buf;
CAN_message_t oil_needle_max_buf, oil_needle_min_buf;
CAN_message_t speedo_needle_release_buf, tacho_needle_release_buf, oil_needle_release_buf;

// Debug LED patterns
void errorBlink(int pattern) {
    while (1) {
        switch (pattern) {
            case 1: // K-CAN error
                for (int i = 0; i < 2; i++) {
                    digitalWrite(13, HIGH); delay(100);
                    digitalWrite(13, LOW); delay(100);
                }
                delay(1000);
                break;
            case 2: // PT-CAN error
                for (int i = 0; i < 3; i++) {
                    digitalWrite(13, HIGH); delay(100);
                    digitalWrite(13, LOW); delay(100);
                }
                delay(1000);
                break;
            default:
                digitalWrite(13, HIGH); delay(500);
                digitalWrite(13, LOW); delay(500);
        }
    }
}

void initializeGaugeMessages() {
    // Speedometer messages (0-325 km/h)
    uint8_t speedo_max[] = {0x60, 0x05, 0x30, 0x20, 0x06, 0x12, 0x11, 0};
    uint8_t speedo_min[] = {0x60, 0x05, 0x30, 0x20, 0x06, 0x00, 0x00, 0};
    uint8_t speedo_release[] = {0x60, 0x05, 0x30, 0x20, 0x06, 0xFF, 0xFF, 0};
    
    // Tachometer messages (0-8000 RPM)
    uint8_t tacho_max[] = {0x60, 0x05, 0x30, 0x21, 0x06, 0x12, 0x3D, 0};
    uint8_t tacho_min[] = {0x60, 0x05, 0x30, 0x21, 0x06, 0x00, 0x00, 0};
    uint8_t tacho_release[] = {0x60, 0x05, 0x30, 0x21, 0x06, 0xFF, 0xFF, 0};
    
    // Oil temperature messages
    uint8_t oil_max[] = {0x60, 0x05, 0x30, 0x23, 0x06, 0x07, 0x12, 0};
    uint8_t oil_min[] = {0x60, 0x05, 0x30, 0x23, 0x06, 0x00, 0x00, 0};
    uint8_t oil_release[] = {0x60, 0x05, 0x30, 0x23, 0x06, 0xFF, 0xFF, 0};

    // Initialize message buffers
    memcpy(speedo_needle_max_buf.buf, speedo_max, 8);
    memcpy(speedo_needle_min_buf.buf, speedo_min, 8);
    memcpy(speedo_needle_release_buf.buf, speedo_release, 8);
    memcpy(tacho_needle_max_buf.buf, tacho_max, 8);
    memcpy(tacho_needle_min_buf.buf, tacho_min, 8);
    memcpy(tacho_needle_release_buf.buf, tacho_release, 8);
    memcpy(oil_needle_max_buf.buf, oil_max, 8);
    memcpy(oil_needle_min_buf.buf, oil_min, 8);
    memcpy(oil_needle_release_buf.buf, oil_release, 8);

    // Set message IDs and lengths
    speedo_needle_max_buf.id = tacho_needle_max_buf.id = oil_needle_max_buf.id = 0x6F1;
    speedo_needle_min_buf.id = tacho_needle_min_buf.id = oil_needle_min_buf.id = 0x6F1;
    speedo_needle_release_buf.id = tacho_needle_release_buf.id = oil_needle_release_buf.id = 0x6F1;
    
    speedo_needle_max_buf.len = tacho_needle_max_buf.len = oil_needle_max_buf.len = 8;
    speedo_needle_min_buf.len = tacho_needle_min_buf.len = oil_needle_min_buf.len = 8;
    speedo_needle_release_buf.len = tacho_needle_release_buf.len = oil_needle_release_buf.len = 8;
}

bool initializeKCAN() {
    KCAN.setClock(FLEXCAN_CLOCK::CLK_24MHz);
    
    // Set explicit pins for K-CAN with TJA1050
    KCAN.setTX(22);
    KCAN.setRX(23);
    
    // Initialize with default settings first
    KCAN.begin();
    delay(250); // Give time for transceiver to stabilize
    
    // Configure for 100kbps with TJA1050 settings
    FLEXCAN_timings_t config;
    config.propseg = 6;
    config.pseg1 = 7;
    config.pseg2 = 4;
    config.rjw = 4;
    config.presdiv = 24;
    KCAN.setBaudRate(config);
    
    KCAN.setMaxMB(16);
    KCAN.enableFIFO();
    KCAN.enableFIFOInterrupt();
    
    delay(100);
    
    // Try to read the bus first
    CAN_message_t msg;
    uint32_t timeout = millis() + 1000;
    while (millis() < timeout) {
        if (KCAN.read(msg)) {
            Serial.println("K-CAN: Bus activity detected");
            return true;
        }
        delay(1);
    }
    
    Serial.println("K-CAN: No bus activity detected, attempting to send test message");
    
    // If no messages, try sending a test message
    CAN_message_t test_msg;
    test_msg.id = 0x100;
    test_msg.len = 8;
    memset(test_msg.buf, 0, 8);
    
    return KCAN.write(test_msg);
}

bool initializePTCAN() {
    PTCAN.setClock(FLEXCAN_CLOCK::CLK_24MHz);
    
    // Set explicit pins for PT-CAN with TJA1050
    PTCAN.setTX(0);
    PTCAN.setRX(1);
    
    // Initialize with default settings first
    PTCAN.begin();
    delay(250); // Give time for transceiver to stabilize
    
    // Configure for 500kbps with TJA1050 settings
    FLEXCAN_timings_t config;
    config.propseg = 6;
    config.pseg1 = 7;
    config.pseg2 = 4;
    config.rjw = 4;
    config.presdiv = 4;
    PTCAN.setBaudRate(config);
    
    PTCAN.setMaxMB(16);
    PTCAN.enableFIFO();
    PTCAN.enableFIFOInterrupt();
    
    delay(100);
    
    // Try to read the bus first
    CAN_message_t msg;
    uint32_t timeout = millis() + 1000;
    while (millis() < timeout) {
        if (PTCAN.read(msg)) {
            Serial.println("PT-CAN: Bus activity detected");
            return true;
        }
        delay(1);
    }
    
    Serial.println("PT-CAN: No bus activity detected, attempting to send test message");
    
    // If no messages, try sending a test message
    CAN_message_t test_msg;
    test_msg.id = 0x100;
    test_msg.len = 8;
    memset(test_msg.buf, 0, 8);
    
    return PTCAN.write(test_msg);
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    
    // Initial LED flash
    for(int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
    }
    
    Serial.begin(115200);
    delay(400);
    
    Serial.println("Starting CAN initialization...");
    Serial.println("Make sure car ignition is ON");
    delay(1000);
    
    Serial.println("Initializing K-CAN (100kbps)...");
    if (!initializeKCAN()) {
        Serial.println("K-CAN initialization failed!");
        Serial.println("Check: \n1. Car ignition ON?\n2. K-CAN connections on pins 22/23?\n3. Power to TJA1050?");
        while(1) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
            delay(400);
        }
    }
    Serial.println("K-CAN initialized successfully");
    
    Serial.println("Initializing PT-CAN (500kbps)...");
    if (!initializePTCAN()) {
        Serial.println("PT-CAN initialization failed!");
        Serial.println("Check: \n1. Car ignition ON?\n2. PT-CAN connections on pins 0/1?\n3. Power to TJA1050?");
        while(1) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
            delay(100);
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
            delay(400);
        }
    }
    Serial.println("PT-CAN initialized successfully");
    
    initializeGaugeMessages();
    
    // Success indication
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Setup complete - monitoring CAN buses");
}

void loop() {
    static uint32_t lastBlink = 0;
    static bool ledState = false;
    
    // Heartbeat LED
    if (millis() - lastBlink >= 2000) {
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState);
        lastBlink = millis();
        Serial.println("System running - CAN buses active");
    }
    
    // Process CAN messages
    CAN_message_t msg;
    
    // Check K-CAN messages
    while (KCAN.read(msg)) {
        Serial.print("K-CAN MSG ID: 0x");
        Serial.print(msg.id, HEX);
        Serial.print(" Data: ");
        for (int i = 0; i < msg.len; i++) {
            Serial.print(msg.buf[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
    
    // Check PT-CAN messages
    while (PTCAN.read(msg)) {
        Serial.print("PT-CAN MSG ID: 0x");
        Serial.print(msg.id, HEX);
        Serial.print(" Data: ");
        for (int i = 0; i < msg.len; i++) {
            Serial.print(msg.buf[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
    
    // Process any pending CAN events
    KCAN.events();
    PTCAN.events();
} 