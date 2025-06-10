#if !defined(ARDUINO) 
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#endif

#include <Arduino.h>
#include <FlexCAN_T4.h>
#include "imxrt_flexcan.h"

// Test CAN1 (pins 22/23)
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> CAN1_Bus;

void printCANStatus(const char* name, FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16>& can) {
    CAN_error_t err;
    can.error(err, false);
    
    Serial.print("\n=== ");
    Serial.print(name);
    Serial.println(" Status ===");
    
    Serial.print("Bus State: ");
    Serial.println((char*)err.state);
    
    Serial.print("Error Counters - RX: ");
    Serial.print(err.RX_ERR_COUNTER);
    Serial.print(" TX: ");
    Serial.println(err.TX_ERR_COUNTER);
    
    Serial.print("Error State: ");
    Serial.println((char*)err.FLT_CONF);

    // Print specific errors if any
    if (err.BIT1_ERR) Serial.println("  Bit1 Error detected");
    if (err.BIT0_ERR) Serial.println("  Bit0 Error detected");
    if (err.ACK_ERR) Serial.println("  Acknowledge Error detected");
    if (err.CRC_ERR) Serial.println("  CRC Error detected");
    if (err.FRM_ERR) Serial.println("  Form Error detected");
    if (err.STF_ERR) Serial.println("  Stuff Error detected");
}

void setup() {
    // LED for visual feedback
    pinMode(LED_BUILTIN, OUTPUT);
    
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== TEST MESSAGE ===");
    Serial.println("If you see this, serial is working!");
    Serial.println("===================");
    
    Serial.println("\nTeensy 4.0 CAN Test - Auto Speed Detection");
    Serial.println("Testing pins 22/23");
    Serial.println("Will alternate between 100kbps and 500kbps");
    
    // Initialize CAN1
    CAN1_Bus.begin();
    CAN1_Bus.setBaudRate(100000); // Start with 100kbps
    CAN1_Bus.setTX(ALT);
    CAN1_Bus.setRX(ALT);
    CAN1_Bus.setMaxMB(16);
    CAN1_Bus.enableFIFO();
    CAN1_Bus.enableFIFOInterrupt();
    
    printCANStatus("Initial", CAN1_Bus);
    Serial.println("\nMonitoring for CAN messages...");
    Serial.println("Make sure ignition is in position II");
    Serial.flush();
}

void loop() {
    static uint32_t lastBlink = 0;
    static uint32_t lastStatus = 0;
    static uint32_t lastSpeedSwitch = 0;
    static uint32_t msgCount = 0;
    static bool using100k = true;
    static uint32_t errors100k = 0;
    static uint32_t errors500k = 0;
    
    // Heartbeat LED
    if (millis() - lastBlink > 500) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        lastBlink = millis();
    }
    
    // Print status every 5 seconds
    if (millis() - lastStatus > 5000) {
        CAN_error_t err;
        CAN1_Bus.error(err, false);
        
        Serial.print("\nSpeed: ");
        Serial.print(using100k ? "100kbps" : "500kbps");
        Serial.print(" Messages: ");
        Serial.print(msgCount);
        Serial.print(" Errors: ");
        Serial.println(using100k ? errors100k : errors500k);
        
        // Update error counts
        if (err.BIT1_ERR || err.BIT0_ERR || err.ACK_ERR || err.CRC_ERR || err.FRM_ERR || err.STF_ERR) {
            if (using100k) errors100k++;
            else errors500k++;
        }
        
        printCANStatus("Current", CAN1_Bus);
        msgCount = 0;
        lastStatus = millis();
    }

    // Switch speeds every 10 seconds
    if (millis() - lastSpeedSwitch > 10000) {
        using100k = !using100k;
        CAN1_Bus.setBaudRate(using100k ? 100000 : 500000);
        Serial.print("\nSwitching to ");
        Serial.print(using100k ? "100kbps" : "500kbps");
        Serial.println("...");
        lastSpeedSwitch = millis();
        
        // Reset message count when switching speeds
        msgCount = 0;
    }
    
    // Check for messages
    CAN_message_t msg;
    if (CAN1_Bus.read(msg)) {
        msgCount++;
        Serial.print("ID: 0x");
        Serial.print(msg.id, HEX);
        Serial.print(" Len: ");
        Serial.print(msg.len);
        Serial.print(" Data: ");
        for (uint8_t i = 0; i < msg.len; i++) {
            if (msg.buf[i] < 0x10) Serial.print("0");
            Serial.print(msg.buf[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
} 