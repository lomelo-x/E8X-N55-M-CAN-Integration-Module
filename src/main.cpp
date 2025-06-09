#include <Arduino.h>
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

// Function to blink LED in a pattern
void blinkLED(int times, int onTime, int offTime) {
    for (int i = 0; i < times; i++) {
        digitalWrite(13, HIGH);
        delay(onTime);
        digitalWrite(13, LOW);
        if (i < times - 1) delay(offTime); // Don't delay after last blink
    }
}

void setup() {
    // Initialize LED pin for debugging
    pinMode(13, OUTPUT);
    
    // Startup pattern - 3 quick blinks
    blinkLED(3, 100, 100);
    delay(500);

    // Initialize Serial and wait for connection
    Serial.begin(115200);
    delay(1000); // Give serial time to start
    Serial.println("Starting CAN initialization...");

    // Configure K-CAN
    Serial.println("Initializing K-CAN...");
    KCAN.setClock(FLEXCAN_CLOCK::CLK_24MHz);
    if (!KCAN.begin()) {
        Serial.println("K-CAN initialization failed!");
        while (1) {
            digitalWrite(13, HIGH);
            delay(100);
            digitalWrite(13, LOW);
            delay(100);
        }
    }
    
    Serial.println("Setting K-CAN baud rate...");
    KCAN.setBaudRate(100000, FLEXCAN_RXTX::TX); // 100kbps
    
    Serial.println("Setting K-CAN pins...");
    KCAN.setTX(FLEXCAN_PINS::ALT); // K-CAN TX pin (22)
    KCAN.setRX(FLEXCAN_PINS::ALT); // K-CAN RX pin (23)
    
    Serial.println("Enabling K-CAN FIFO...");
    KCAN.enableFIFO();
    KCAN.enableFIFOInterrupt();
    Serial.println("K-CAN initialization complete");
    
    // Configure PT-CAN
    Serial.println("Initializing PT-CAN...");
    PTCAN.setClock(FLEXCAN_CLOCK::CLK_24MHz);
    if (!PTCAN.begin()) {
        Serial.println("PT-CAN initialization failed!");
        while (1) {
            digitalWrite(13, HIGH);
            delay(100);
            digitalWrite(13, LOW);
            delay(100);
        }
    }
    
    Serial.println("Setting PT-CAN baud rate...");
    PTCAN.setBaudRate(500000, FLEXCAN_RXTX::TX); // 500kbps
    
    Serial.println("Setting PT-CAN pins...");
    PTCAN.setTX(FLEXCAN_PINS::DEF); // PT-CAN TX pin (0)
    PTCAN.setRX(FLEXCAN_PINS::DEF); // PT-CAN RX pin (1)
    
    Serial.println("Enabling PT-CAN FIFO...");
    PTCAN.enableFIFO();
    PTCAN.enableFIFOInterrupt();
    Serial.println("PT-CAN initialization complete");
    
    // Initialize gauge messages
    Serial.println("Initializing gauge messages...");
    initializeGaugeMessages();
    Serial.println("Gauge messages initialized");
    
    // Success pattern - one long blink
    Serial.println("Setup complete - showing success blink");
    blinkLED(1, 1000, 0);
    Serial.println("Entering main loop");
}

void loop() {
    static uint32_t lastBlink = 0;
    static bool ledState = false;
    static uint32_t lastStatus = 0;
    
    // Print status every 5 seconds
    if (millis() - lastStatus >= 5000) {
        Serial.println("System running - checking CAN buses");
        lastStatus = millis();
    }

    // Blink LED every second to show the program is running
    if (millis() - lastBlink >= 1000) {
        ledState = !ledState;
        digitalWrite(13, ledState);
        lastBlink = millis();
    }

    // Check K-CAN messages
    CAN_message_t msg;
    if (KCAN.read(msg)) {
        Serial.println("K-CAN message received!");
        Serial.print("ID: 0x");
        Serial.print(msg.id, HEX);
        Serial.print(" Data: ");
        for (int i = 0; i < msg.len; i++) {
            Serial.print(msg.buf[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        
        // Visual indicator on LED for received message
        digitalWrite(13, HIGH);
        delay(10);
        digitalWrite(13, LOW);
    }
    
    // Check PT-CAN messages
    if (PTCAN.read(msg)) {
        Serial.println("PT-CAN message received!");
        Serial.print("ID: 0x");
        Serial.print(msg.id, HEX);
        Serial.print(" Data: ");
        for (int i = 0; i < msg.len; i++) {
            Serial.print(msg.buf[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        
        // Visual indicator on LED for received message
        digitalWrite(13, HIGH);
        delay(10);
        digitalWrite(13, LOW);
    }
    
    // Process any pending CAN events
    KCAN.events();
    PTCAN.events();
} 