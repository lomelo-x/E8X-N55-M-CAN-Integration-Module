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
}

void loop() {
  // Send a test message from CAN2 every second
  static uint32_t lastSend = 0;
  if (millis() - lastSend > 1000) {
    CAN_message_t outMsg;
    outMsg.id = 0x123;
    outMsg.len = 8;
    for (int i = 0; i < 8; i++) outMsg.buf[i] = i;
    if (can2.write(outMsg)) {
      Serial.println("Message queued for sending on CAN2");
    } else {
      Serial.println("Failed to queue message on CAN2");
    }
    lastSend = millis();
  }

  // Read from CAN1
  if (can1.read(msg)) {
    Serial.print("CAN1 ");
    Serial.print("MB: "); Serial.print(msg.mb);
    Serial.print("  ID: 0x"); Serial.print(msg.id, HEX );
    Serial.print("  EXT: "); Serial.print(msg.flags.extended );
    Serial.print("  LEN: "); Serial.print(msg.len);
    Serial.print(" DATA: ");
    for (uint8_t i = 0; i < msg.len; i++) {
      Serial.print(msg.buf[i]); Serial.print(" ");
    }
    Serial.print("  TS: "); Serial.println(msg.timestamp);
  }
}