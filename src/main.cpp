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
    Serial.println("K-CAN: Setting up pins and clock...");
    KCAN.setClock(FLEXCAN_CLOCK::CLK_24MHz);
    KCAN.setTX(FLEXCAN_PINS::ALT);  // Pin 22
    KCAN.setRX(FLEXCAN_PINS::ALT);  // Pin 23
    
    Serial.println("K-CAN: Starting initialization...");
    KCAN.begin();
    delay(250); // Give time for transceiver to stabilize
    
    // Start with lower baud rate in listen-only mode
    Serial.println("K-CAN: Setting initial parameters...");
    KCAN.setBaudRate(95000, FLEXCAN_RXTX::LISTEN_ONLY);
    KCAN.setMaxMB(16);
    KCAN.enableFIFO();
    KCAN.enableFIFOInterrupt();
    
    Serial.println("K-CAN: Listening for bus activity...");
    
    // Try to read the bus first
    CAN_message_t msg;
    uint32_t timeout = millis() + 2000; // Extended timeout
    bool activity = false;
    
    while (millis() < timeout) {
        if (KCAN.read(msg)) {
            Serial.print("K-CAN: Message received! ID: 0x");
            Serial.print(msg.id, HEX);
            Serial.print(" Length: ");
            Serial.println(msg.len);
            activity = true;
            break;
        }
        delay(1);
    }
    
    if (!activity) {
        Serial.println("K-CAN: No messages detected in listen mode");
        
        // Switch to normal mode and try standard speed
        Serial.println("K-CAN: Switching to normal mode...");
        KCAN.setBaudRate(100000);
        delay(100);
        
        // Send test message
        CAN_message_t test_msg;
        test_msg.id = 0x100;
        test_msg.len = 8;
        memset(test_msg.buf, 0, 8);
        
        if (!KCAN.write(test_msg)) {
            Serial.println("K-CAN: Failed to send test message");
            return false;
        }
        Serial.println("K-CAN: Test message sent successfully");
    }
    
    return true;
}

bool initializePTCAN() {
    Serial.println("PT-CAN: Setting up pins and clock...");
    PTCAN.setClock(FLEXCAN_CLOCK::CLK_24MHz);
    PTCAN.setTX(FLEXCAN_PINS::DEF);  // Pin 0
    PTCAN.setRX(FLEXCAN_PINS::DEF);  // Pin 1
    
    Serial.println("PT-CAN: Starting initialization...");
    PTCAN.begin();
    delay(250); // Give time for transceiver to stabilize
    
    // Start with slightly lower baud rate in listen-only mode
    Serial.println("PT-CAN: Setting initial parameters...");
    PTCAN.setBaudRate(485000, FLEXCAN_RXTX::LISTEN_ONLY);
    PTCAN.setMaxMB(16);
    PTCAN.enableFIFO();
    PTCAN.enableFIFOInterrupt();
    
    Serial.println("PT-CAN: Listening for bus activity...");
    
    // Try to read the bus first
    CAN_message_t msg;
    uint32_t timeout = millis() + 2000; // Extended timeout
    bool activity = false;
    
    while (millis() < timeout) {
        if (PTCAN.read(msg)) {
            Serial.print("PT-CAN: Message received! ID: 0x");
            Serial.print(msg.id, HEX);
            Serial.print(" Length: ");
            Serial.println(msg.len);
            activity = true;
            break;
        }
        delay(1);
    }
    
    if (!activity) {
        Serial.println("PT-CAN: No messages detected in listen mode");
        
        // Switch to normal mode and try standard speed
        Serial.println("PT-CAN: Switching to normal mode...");
        PTCAN.setBaudRate(500000);
        delay(100);
        
        // Send test message
        CAN_message_t test_msg;
        test_msg.id = 0x100;
        test_msg.len = 8;
        memset(test_msg.buf, 0, 8);
        
        if (!PTCAN.write(test_msg)) {
            Serial.println("PT-CAN: Failed to send test message");
            return false;
        }
        Serial.println("PT-CAN: Test message sent successfully");
    }
    
    return true;
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    
    // Power up indication
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    
    // Start Serial
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\nStarting minimal CAN test...");
    
    // Serial ready indication
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    
    // Try to initialize CAN
    Serial.println("Setting up K-CAN...");
    
    KCAN.begin();  // Start with just this
    
    Serial.println("K-CAN begin() completed");
    
    // CAN ready indication
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    
    Serial.println("Setup complete!");
}

void loop() {
    static uint32_t lastBlink = 0;
    
    // Heartbeat
    if (millis() - lastBlink >= 2000) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("Heartbeat");
        lastBlink = millis();
    }
    
    // Just try to read messages
    CAN_message_t msg;
    if (KCAN.read(msg)) {
        Serial.print("Got message, ID: 0x");
        Serial.println(msg.id, HEX);
    }
} 