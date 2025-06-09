#if !defined(ARDUINO) 
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#endif

#include <Arduino.h>
#include <FlexCAN_T4.h>

// Try both speeds on red bundle (pins 22/23)
FlexCAN_T4<CAN_DEV_TABLE::CAN1, RX_SIZE_256, TX_SIZE_16> CAN1_Bus;

// Try both speeds on blue bundle (pins 0/1)
FlexCAN_T4<CAN_DEV_TABLE::CAN2, RX_SIZE_256, TX_SIZE_16> CAN2_Bus;

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

template<const CAN_DEV_TABLE _bus, const FLEXCAN_RXQUEUE_TABLE _rxSize = RX_SIZE_256, const FLEXCAN_TXQUEUE_TABLE _txSize = TX_SIZE_16>
void printCANStatus(const char* name, FlexCAN_T4<_bus, _rxSize, _txSize>& can) {
    CAN_error_t err;
    can.error(err, false);  // false = don't print details in the function
    Serial.print(name);
    Serial.print(" Status - RX_ERR: ");
    Serial.print(err.RX_ERR_COUNTER);
    Serial.print(" TX_ERR: ");
    Serial.print(err.TX_ERR_COUNTER);
    Serial.print(" BIT1_ERR: ");
    Serial.print(err.BIT1_ERR);
    Serial.print(" BIT0_ERR: ");
    Serial.println(err.BIT0_ERR);
}

// Add status monitoring function
void printCANStatus(const char* name, FlexCAN_T4<CAN_DEV_TABLE::CAN1, RX_SIZE_256, TX_SIZE_16>& can1, FlexCAN_T4<CAN_DEV_TABLE::CAN2, RX_SIZE_256, TX_SIZE_16>& can2) {
    CAN_error_t err;
    
    // Check CAN1 status
    can1.error(err, false);
    Serial.print(name);
    Serial.print(" Status - RX_ERR: ");
    Serial.print(err.RX_ERR_COUNTER);
    Serial.print(" TX_ERR: ");
    Serial.print(err.TX_ERR_COUNTER);
    Serial.print(" State: ");
    Serial.print((char*)err.state);
    Serial.print(" FLT_CONF: ");
    Serial.println((char*)err.FLT_CONF);
    
    // Check for specific errors
    if (err.BIT1_ERR) Serial.println("  Bit1 Error detected");
    if (err.BIT0_ERR) Serial.println("  Bit0 Error detected");
    if (err.ACK_ERR) Serial.println("  Acknowledge Error detected");
    if (err.CRC_ERR) Serial.println("  CRC Error detected");
    if (err.FRM_ERR) Serial.println("  Form Error detected");
    if (err.STF_ERR) Serial.println("  Stuff Error detected");

    // Check CAN2 status
    can2.error(err, false);
    Serial.print(name);
    Serial.print(" CAN2 Status - RX_ERR: ");
    Serial.print(err.RX_ERR_COUNTER);
    Serial.print(" TX_ERR: ");
    Serial.print(err.TX_ERR_COUNTER);
    Serial.print(" State: ");
    Serial.print((char*)err.state);
    Serial.print(" FLT_CONF: ");
    Serial.println((char*)err.FLT_CONF);
    
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
    delay(1000); // Give more time for serial to initialize
    
    Serial.println("\nTeensy 4.0 CAN Test - Bus Identification");
    Serial.println("Initializing...");

    // First try: Red bundle at 100kbps
    Serial.println("\nTesting Red bundle (pins 22/23) at 100kbps...");
    CAN1_Bus.begin();
    CAN1_Bus.setClock(CLK_24MHz);
    CAN1_Bus.setBaudRate(100000);
    CAN1_Bus.setMaxMB(16);
    CAN1_Bus.enableFIFO();
    CAN1_Bus.enableFIFOInterrupt();
    Serial.println("Red bundle initialized");

    delay(100); // Small delay between initializations

    // First try: Blue bundle at 500kbps
    Serial.println("Testing Blue bundle (pins 0/1) at 500kbps...");
    CAN2_Bus.begin();
    CAN2_Bus.setClock(CLK_24MHz);
    CAN2_Bus.setBaudRate(500000);
    CAN2_Bus.setMaxMB(16);
    CAN2_Bus.enableFIFO();
    CAN2_Bus.enableFIFOInterrupt();
    Serial.println("Blue bundle initialized");

    // Print initial bus status
    printCANStatus("Initial", CAN1_Bus, CAN2_Bus);

    Serial.println("\nMonitoring both buses - looking for messages...");
    Serial.println("Will show which speed works for each bundle.");
    Serial.flush(); // Ensure all serial data is sent
}

void loop() {
    static uint32_t lastSwitch = 0;
    static uint32_t lastBlink = 0;
    static uint32_t lastStatus = 0;
    static uint32_t lastCount = 0;
    static bool firstConfig = true;
    static uint32_t redMsgCount = 0;
    static uint32_t blueMsgCount = 0;

    // Heartbeat LED
    if (millis() - lastBlink > 500) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        lastBlink = millis();
    }

    // Print bus status every 5 seconds
    if (millis() - lastStatus > 5000) {
        printCANStatus("Current", CAN1_Bus, CAN2_Bus);
        lastStatus = millis();
    }

    // Switch configurations after 10 seconds if no messages
    if (millis() - lastSwitch > 10000 && firstConfig) {
        firstConfig = false;
        Serial.println("\nSwitching configurations...");
        Serial.println("Testing Red bundle at 500kbps...");
        CAN1_Bus.setBaudRate(500000);
        
        Serial.println("Testing Blue bundle at 100kbps...");
        CAN2_Bus.setBaudRate(100000);
        
        lastSwitch = millis();
        redMsgCount = 0;
        blueMsgCount = 0;
        Serial.flush();
    }

    // Check Red bundle (CAN1)
    CAN_message_t msg;
    if (CAN1_Bus.read(msg)) {
        redMsgCount++;
        Serial.print("Red(22/23) 0x");
        Serial.print(msg.id, HEX);
        Serial.print(": ");
        for (uint8_t i = 0; i < msg.len; i++) {
            if (msg.buf[i] < 0x10) Serial.print("0");
            Serial.print(msg.buf[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        Serial.flush();
    }

    // Check Blue bundle (CAN2)
    if (CAN2_Bus.read(msg)) {
        blueMsgCount++;
        Serial.print("Blue(0/1) 0x");
        Serial.print(msg.id, HEX);
        Serial.print(": ");
        for (uint8_t i = 0; i < msg.len; i++) {
            if (msg.buf[i] < 0x10) Serial.print("0");
            Serial.print(msg.buf[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        Serial.flush();
    }

    // Print message counts every 2 seconds
    if (millis() - lastCount > 2000) {
        Serial.print("\nCounts - Red: ");
        Serial.print(redMsgCount);
        Serial.print(" Blue: ");
        Serial.println(blueMsgCount);
        Serial.print("Config: ");
        Serial.print(firstConfig ? "1st" : "2nd");
        Serial.print(" Red: ");
        Serial.print(firstConfig ? "100k" : "500k");
        Serial.print(" Blue: ");
        Serial.println(firstConfig ? "500k" : "100k");
        Serial.flush();
        lastCount = millis();
    }

    // Process CAN events
    CAN1_Bus.events();
    CAN2_Bus.events();
} 