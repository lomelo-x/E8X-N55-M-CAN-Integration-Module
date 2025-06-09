#if !defined(ARDUINO) 
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#endif

#include <Arduino.h>
#include <FlexCAN_T4.h>

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> KCAN;  // K-CAN on pins 22/23
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> PTCAN; // PT-CAN on pins 0/1

// LED patterns
void blinkPattern(int times, int onTime, int offTime) {
    for(int i = 0; i < times; i++) {
        digitalWrite(13, HIGH);
        delay(onTime);
        digitalWrite(13, LOW);
        if (i < times - 1) {  // Don't delay after last blink
            delay(offTime);
        }
    }
}

void canSniff(const CAN_message_t &msg) {
    Serial.print("MB: "); Serial.print(msg.mb);
    Serial.print(" ID: 0x"); Serial.print(msg.id, HEX);
    Serial.print(" EXT: "); Serial.print(msg.flags.extended);
    Serial.print(" LEN: "); Serial.print(msg.len);
    Serial.print(" DATA: ");
    for (uint8_t i = 0; i < msg.len; i++) {
        Serial.print(msg.buf[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

void setup() {
    // Initialize LED
    pinMode(13, OUTPUT);
    
    // Quick blink to show we're starting
    blinkPattern(2, 100, 100);
    
    // Initialize Serial
    Serial.begin(115200);
    delay(400);
    
    // Another blink to show Serial is ready
    blinkPattern(1, 500, 0);
    
    Serial.println("\nTeensy 4.0 CAN Test");
    Serial.println("Initializing...");
    
    Serial.println("Initializing CAN buses...");

    // Initialize K-CAN (100kbps)
    KCAN.begin();
    KCAN.setClock(CLK_24MHz);
    KCAN.setBaudRate(100000);
    KCAN.setMaxMB(16);
    KCAN.enableFIFO();
    KCAN.enableFIFOInterrupt();
    KCAN.onReceive(canSniff);
    KCAN.mailboxStatus();

    // Initialize PT-CAN (500kbps)
    PTCAN.begin();
    PTCAN.setClock(CLK_24MHz);
    PTCAN.setBaudRate(500000);
    PTCAN.setMaxMB(16);
    PTCAN.enableFIFO();
    PTCAN.enableFIFOInterrupt();
    PTCAN.onReceive(canSniff);
    PTCAN.mailboxStatus();

    // Success pattern - one long blink
    blinkPattern(1, 1000, 0);
    
    Serial.println("CAN buses initialized!");
}

void loop() {
    static uint32_t lastBlink = 0;
    static uint32_t lastStatus = 0;
    
    // Heartbeat every 2 seconds
    if (millis() - lastBlink >= 2000) {
        blinkPattern(1, 50, 0);
        lastBlink = millis();
    }
    
    // Print status every 5 seconds
    if (millis() - lastStatus >= 5000) {
        Serial.println("System running...");
        lastStatus = millis();
    }
    
    // Check for K-CAN messages
    CAN_message_t msg;
    if (KCAN.read(msg)) {
        Serial.print("K-CAN ID: 0x");
        Serial.print(msg.id, HEX);
        Serial.print(" Data: ");
        for (uint8_t i = 0; i < msg.len; i++) {
            Serial.print(msg.buf[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }

    // Check for PT-CAN messages
    if (PTCAN.read(msg)) {
        Serial.print("PT-CAN ID: 0x");
        Serial.print(msg.id, HEX);
        Serial.print(" Data: ");
        for (uint8_t i = 0; i < msg.len; i++) {
            Serial.print(msg.buf[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }

    // Process CAN events
    KCAN.events();
    PTCAN.events();
} 