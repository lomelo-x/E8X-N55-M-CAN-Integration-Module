#include <Arduino.h>
#include "CANHandler.h"

CANHandler canHandler;

// Message IDs for BMW E8X/N55
const uint32_t MSG_GAUGE_ID = 0x6F1;    // Gauge cluster message
const uint32_t MSG_M_BUTTON_ID = 0x1D9; // M button status
const uint32_t MSG_SPORT_ID = 0x1D2;    // Sport display control
const uint32_t MSG_M_LIGHT_ID = 0x399;  // M light control

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {} // Wait for Serial, timeout after 3s
    
    Serial.println("BMW E8X N55 CAN Integration Module");
    Serial.println("Initializing CAN buses...");
    
    if (!canHandler.begin()) {
        Serial.println("CAN Init Failed!");
        while (1) {
            delay(100);
        }
    }
    Serial.println("CAN Initialized Successfully!");
    Serial.println("PT-CAN: 500kbps on CAN1");
    Serial.println("K-CAN: 100kbps on CAN2");
}

void handleMButton() {
    uint32_t rxId;
    uint8_t rxLen;
    uint8_t rxBuf[8];
    
    // Check for M button press on K-CAN
    if (canHandler.readKCAN(rxId, rxBuf, rxLen)) {
        if (rxId == MSG_M_BUTTON_ID) {
            // Check for M button press pattern (BF 7F)
            if (rxLen >= 2 && rxBuf[0] == 0xBF && rxBuf[1] == 0x7F) {
                Serial.println("M Button Pressed!");
                
                // Send Sport display control message
                uint8_t sportMsg[8] = {0x00}; // Fill with proper data
                canHandler.writeKCAN(MSG_SPORT_ID, sportMsg, 8);
                
                // Send M light control message
                uint8_t lightMsg[8] = {0x00}; // Fill with proper data
                canHandler.writeKCAN(MSG_M_LIGHT_ID, lightMsg, 8);
            }
        }
    }
}

void updateGauges() {
    static uint32_t lastUpdate = 0;
    const uint32_t UPDATE_INTERVAL = 100; // Update every 100ms
    
    if (millis() - lastUpdate >= UPDATE_INTERVAL) {
        lastUpdate = millis();
        
        // Example: Send gauge cluster message on PT-CAN
        uint8_t gaugeMsg[8] = {
            0x00, // Speed LSB
            0x00, // Speed MSB
            0x00, // RPM LSB
            0x00, // RPM MSB
            0x00, // Oil temp
            0x00, // Reserved
            0x00, // Reserved
            0x00  // Reserved
        };
        
        canHandler.writePTCAN(MSG_GAUGE_ID, gaugeMsg, 8);
    }
}

void loop() {
    handleMButton();
    updateGauges();
    
    // Optional: Print received messages for debugging
    uint32_t rxId;
    uint8_t rxLen;
    uint8_t rxBuf[8];
    
    // Print PT-CAN messages
    if (canHandler.readPTCAN(rxId, rxBuf, rxLen)) {
        Serial.print("PT-CAN ID: 0x");
        Serial.print(rxId, HEX);
        Serial.print(" Len: ");
        Serial.print(rxLen);
        Serial.print(" Data: ");
        for (int i = 0; i < rxLen; i++) {
            Serial.print(rxBuf[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
    
    // Print K-CAN messages
    if (canHandler.readKCAN(rxId, rxBuf, rxLen)) {
        Serial.print("K-CAN ID: 0x");
        Serial.print(rxId, HEX);
        Serial.print(" Len: ");
        Serial.print(rxLen);
        Serial.print(" Data: ");
        for (int i = 0; i < rxLen; i++) {
            Serial.print(rxBuf[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
} 