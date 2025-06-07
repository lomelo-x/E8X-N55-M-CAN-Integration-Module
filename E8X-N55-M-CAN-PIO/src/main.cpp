#include <Arduino.h>
#include <FlexCAN_T4.h>

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> KCAN; // K-CAN (100kbps)
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> PTCAN; // PT-CAN (500kbps)

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

void performGaugeSweep() {
    Serial.println("Starting gauge sweep animation...");
    
    // Sweep to max
    KCAN.write(speedo_needle_max_buf);
    delay(100);
    KCAN.write(tacho_needle_max_buf);
    delay(100);
    KCAN.write(oil_needle_max_buf);
    delay(300);

    // Hold at max
    delay(500);

    // Sweep to min
    KCAN.write(tacho_needle_min_buf);
    delay(100);
    KCAN.write(speedo_needle_min_buf);
    delay(100);
    KCAN.write(oil_needle_min_buf);
    delay(300);

    // Hold at min
    delay(500);

    // Release control back to cluster
    KCAN.write(speedo_needle_release_buf);
    delay(100);
    KCAN.write(tacho_needle_release_buf);
    delay(100);
    KCAN.write(oil_needle_release_buf);
    delay(100);

    // Send release commands again to ensure proper operation
    delay(500);
    KCAN.write(tacho_needle_release_buf);
    delay(100);
    KCAN.write(speedo_needle_release_buf);
    delay(100);
    KCAN.write(oil_needle_release_buf);
    
    Serial.println("Gauge sweep complete");
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) ; // Wait for serial or timeout
    Serial.println("E8X N55 M-CAN Integration Module Starting...");

    // Configure K-CAN
    KCAN.begin();
    KCAN.setBaudRate(100000); // 100kbps
    KCAN.enableFIFO();
    KCAN.enableFIFOInterrupt();

    // Configure PT-CAN
    PTCAN.begin();
    PTCAN.setBaudRate(500000); // 500kbps
    PTCAN.enableFIFO();
    PTCAN.enableFIFOInterrupt();

    // Initialize gauge messages
    initializeGaugeMessages();

    Serial.println("CAN buses initialized");
    
    // Perform initial gauge sweep
    performGaugeSweep();
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
}