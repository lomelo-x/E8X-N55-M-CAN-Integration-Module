#include "can_core.h"

// Global CAN objects
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> KCAN;
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> PTCAN;
CAN_message_t k_msg;
CAN_message_t pt_msg;

// CAN receive callbacks
void kcan_receive_callback(const CAN_message_t &msg) {
    k_msg = msg;  // Store in global buffer
    Serial.print("K-CAN ID: 0x");
    Serial.println(msg.id, HEX);
}

void ptcan_receive_callback(const CAN_message_t &msg) {
    pt_msg = msg;  // Store in global buffer
    Serial.print("PT-CAN ID: 0x");
    Serial.println(msg.id, HEX);
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
    CAN_message_t speedo_needle_max_buf, speedo_needle_min_buf;
    CAN_message_t tacho_needle_max_buf, tacho_needle_min_buf;
    CAN_message_t oil_needle_max_buf, oil_needle_min_buf;
    CAN_message_t speedo_needle_release_buf, tacho_needle_release_buf, oil_needle_release_buf;

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
    CAN_message_t speedo_needle_max_buf, speedo_needle_min_buf;
    CAN_message_t tacho_needle_max_buf, tacho_needle_min_buf;
    CAN_message_t oil_needle_max_buf, oil_needle_min_buf;
    CAN_message_t speedo_needle_release_buf, tacho_needle_release_buf, oil_needle_release_buf;

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