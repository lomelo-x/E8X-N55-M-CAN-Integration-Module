#include <FlexCAN_T4.h>

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can1;
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> can2;
CAN_message_t msg;

// Example gauge sweep message buffers (update these as needed for your cluster)
CAN_message_t speedo_needle_min_buf;
CAN_message_t speedo_needle_max_buf;
CAN_message_t tacho_needle_min_buf;
CAN_message_t tacho_needle_max_buf;
CAN_message_t oil_needle_min_buf;
CAN_message_t oil_needle_max_buf;

#define HEARTBEAT_LED 13

bool rapid_blink = false;

void initializeGaugeMessages() {
    speedo_needle_min_buf.id = 0x6F1;
    speedo_needle_min_buf.len = 8;
    uint8_t speedo_min[8] = {0x60, 0x05, 0x30, 0x20, 0x06, 0x00, 0x00, 0};
    memcpy(speedo_needle_min_buf.buf, speedo_min, 8);

    speedo_needle_max_buf.id = 0x6F1;
    speedo_needle_max_buf.len = 8;
    uint8_t speedo_max[8] = {0x60, 0x05, 0x30, 0x20, 0x06, 0x12, 0x11, 0};
    memcpy(speedo_needle_max_buf.buf, speedo_max, 8);

    tacho_needle_min_buf.id = 0x6F1;
    tacho_needle_min_buf.len = 8;
    uint8_t tacho_min[8] = {0x60, 0x05, 0x30, 0x21, 0x06, 0x00, 0x00, 0};
    memcpy(tacho_needle_min_buf.buf, tacho_min, 8);

    tacho_needle_max_buf.id = 0x6F1;
    tacho_needle_max_buf.len = 8;
    uint8_t tacho_max[8] = {0x60, 0x05, 0x30, 0x21, 0x06, 0x12, 0x3D, 0};
    memcpy(tacho_needle_max_buf.buf, tacho_max, 8);

    oil_needle_min_buf.id = 0x6F1;
    oil_needle_min_buf.len = 8;
    uint8_t oil_min[8] = {0x60, 0x05, 0x30, 0x23, 0x06, 0x00, 0x00, 0};
    memcpy(oil_needle_min_buf.buf, oil_min, 8);

    oil_needle_max_buf.id = 0x6F1;
    oil_needle_max_buf.len = 8;
    uint8_t oil_max[8] = {0x60, 0x05, 0x30, 0x23, 0x06, 0x07, 0x12, 0};
    memcpy(oil_needle_max_buf.buf, oil_max, 8);
}

void performGaugeSweep() {
    Serial.println("Sweeping gauges...");
    rapid_blink = true;
    // Sweep to min
    can1.write(speedo_needle_min_buf);
    can1.write(tacho_needle_min_buf);
    can1.write(oil_needle_min_buf);
    delay(100);
    // Sweep to max
    can1.write(speedo_needle_max_buf);
    can1.write(tacho_needle_max_buf);
    can1.write(oil_needle_max_buf);
    delay(100);
    // Sweep back to min
    can1.write(speedo_needle_min_buf);
    can1.write(tacho_needle_min_buf);
    can1.write(oil_needle_min_buf);
    delay(100);
    rapid_blink = false;
    Serial.println("Gauge sweep complete");
}

void setup(void) {
    Serial.begin(115200);
    Serial.println("E8X-M-CAN Initializing...");
    can1.begin();
    can1.setBaudRate(125000); // Set to your K-CAN baud rate if needed
    can2.begin();
    can2.setBaudRate(125000); // Set to your PT-CAN baud rate if needed
    delay(100);
    initializeGaugeMessages();
    performGaugeSweep();

    pinMode(HEARTBEAT_LED, OUTPUT); // Set LED pin as output
}

void loop() {
    static uint32_t lastMillis = 0;
    static uint8_t state = 0;
    static bool ledState = false;
    uint32_t now = millis();

    if (rapid_blink) {
        // Rapid blink: 50ms on, 50ms off
        if (now - lastMillis >= 50) {
            ledState = !ledState;
            digitalWrite(HEARTBEAT_LED, ledState ? HIGH : LOW);
            lastMillis = now;
        }
    } else {
        // Double pulse heartbeat
        switch (state) {
            case 0: digitalWrite(HEARTBEAT_LED, HIGH);
                    if (now - lastMillis >= 150) { lastMillis = now; state = 1; } break;
            case 1: digitalWrite(HEARTBEAT_LED, LOW);
                    if (now - lastMillis >= 150) { lastMillis = now; state = 2; } break;
            case 2: digitalWrite(HEARTBEAT_LED, HIGH);
                    if (now - lastMillis >= 150) { lastMillis = now; state = 3; } break;
            case 3: digitalWrite(HEARTBEAT_LED, LOW);
                    if (now - lastMillis >= 1500) { lastMillis = now; state = 0; } break;
        }
    }
}