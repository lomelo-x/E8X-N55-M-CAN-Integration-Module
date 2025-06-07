#include <Arduino.h>
#include <FlexCAN_T4.h>

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> KCAN; // K-CAN (100kbps)
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> PTCAN; // PT-CAN (500kbps)

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

    Serial.println("CAN buses initialized");
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