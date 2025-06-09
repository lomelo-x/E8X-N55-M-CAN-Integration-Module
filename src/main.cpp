#if !defined(ARDUINO) 
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#endif

#include <Arduino.h>
#include <FlexCAN_T4.h>
#include "imxrt_flexcan.h"

// Only test CAN1 (pins 22/23) first
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
    
    Serial.println("\nTeensy 4.0 CAN Test - PT-CAN Test");
    Serial.println("Testing pins 22/23 at 500kbps (PT-CAN)");

    // Initialize CAN1 for PT-CAN
    CAN1_Bus.begin();
    CAN1_Bus.setBaudRate(500000); // PT-CAN runs at 500kbps
    CAN1_Bus.setMaxMB(16);
    CAN1_Bus.enableFIFO();
    CAN1_Bus.enableFIFOInterrupt();
    
    printCANStatus("Initial", CAN1_Bus);
    Serial.println("\nMonitoring for PT-CAN messages...");
    Serial.println("Make sure ignition is in position II");
    Serial.flush();
}

void loop() {
    static uint32_t lastBlink = 0;
    static uint32_t lastStatus = 0;
    static uint32_t msgCount = 0;
    static uint32_t errorCount = 0;
    
    // Heartbeat LED
    if (millis() - lastBlink > 500) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        lastBlink = millis();
    }
    
    // Print status every 5 seconds
    if (millis() - lastStatus > 5000) {
        CAN_error_t err;
        CAN1_Bus.error(err, false);
        
        Serial.print("\nPT-CAN Status - Messages: ");
        Serial.print(msgCount);
        Serial.print(" Errors: ");
        Serial.println(errorCount);
        
        // Update error count
        if (err.BIT1_ERR || err.BIT0_ERR || err.ACK_ERR || err.CRC_ERR || err.FRM_ERR || err.STF_ERR) {
            errorCount++;
        }
        
        printCANStatus("Current", CAN1_Bus);
        msgCount = 0;
        lastStatus = millis();
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