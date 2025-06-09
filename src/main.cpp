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

// Logging helper function
void log_msg(const char* msg) {
    Serial.println(msg);
    Serial.flush(); // Make sure message is sent
    delay(10);     // Small delay to ensure message is complete
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    
    // Power up indication
    log_msg("\n\n--- STARTUP ---");
    log_msg("1. LED Test starting...");
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    log_msg("LED Test complete");
    
    // Start Serial
    log_msg("2. Initializing Serial...");
    Serial.begin(115200);
    delay(500);
    log_msg("Serial initialized at 115200 baud");
    
    // Serial ready indication
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    
    // CAN Setup
    log_msg("\n--- CAN SETUP ---");
    log_msg("3. Setting up K-CAN...");
    
    // Set clock first
    log_msg("3.1 Setting CAN clock...");
    KCAN.setClock(FLEXCAN_CLOCK::CLK_24MHz);
    log_msg("Clock set to 24MHz");
    
    // Set pins
    log_msg("3.2 Setting CAN pins...");
    KCAN.setTX(FLEXCAN_PINS::ALT); // Pin 22
    KCAN.setRX(FLEXCAN_PINS::ALT); // Pin 23
    log_msg("Pins set: TX=22, RX=23");
    
    // Begin CAN
    log_msg("3.3 Starting CAN initialization...");
    KCAN.begin();
    log_msg("CAN begin() completed");
    
    // Try basic configuration
    log_msg("3.4 Setting basic parameters...");
    KCAN.setBaudRate(100000);
    log_msg("Baud rate set to 100kbps");
    
    // CAN ready indication
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    
    log_msg("\n--- SETUP COMPLETE ---");
    log_msg("Entering main loop...");
}

void loop() {
    static uint32_t lastBlink = 0;
    static uint32_t messageCount = 0;
    static uint32_t lastStatus = 0;
    
    // Heartbeat and status
    if (millis() - lastBlink >= 2000) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
        
        // Print status every 2 seconds
        char status[100];
        snprintf(status, sizeof(status), 
                "Status - Uptime: %lus, Messages received: %lu", 
                millis()/1000, messageCount);
        log_msg(status);
        
        lastBlink = millis();
    }
    
    // Check for CAN messages
    CAN_message_t msg;
    if (KCAN.read(msg)) {
        messageCount++;
        
        // Log message details
        char msgLog[100];
        snprintf(msgLog, sizeof(msgLog), 
                "CAN MSG - ID: 0x%03X, Len: %d, Data: ", 
                msg.id, msg.len);
        Serial.print(msgLog);
        
        // Print message data bytes
        for (uint8_t i = 0; i < msg.len; i++) {
            char byteStr[4];
            snprintf(byteStr, sizeof(byteStr), "%02X ", msg.buf[i]);
            Serial.print(byteStr);
        }
        Serial.println();
    }
    
    // Process CAN events
    KCAN.events();
} 