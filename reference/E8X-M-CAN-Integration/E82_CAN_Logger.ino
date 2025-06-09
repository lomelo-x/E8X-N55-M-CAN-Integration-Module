#include <FlexCAN_T4.h>

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> PTCAN;  // Engine CAN
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> KCAN;   // Interior CAN

// Logging control
bool logPTCAN = true;   // Set to true to log PT-CAN messages
bool logKCAN = false;   // Set to true to log K-CAN messages
unsigned long lastPrint = 0;
const unsigned long PRINT_INTERVAL = 100; // Print every 100ms

void setup() {
  Serial.begin(115200);
  while (!Serial) ; // Wait for serial port
  
  Serial.println("BMW E82 N55 CAN Logger");
  Serial.println("Format: BUS_TYPE ID LENGTH DATA_BYTES");
  
  // Initialize PT-CAN
  PTCAN.begin();
  PTCAN.setBaudRate(500000);
  PTCAN.setMaxMB(16);
  PTCAN.enableFIFO();
  PTCAN.enableFIFOInterrupt();
  
  // Initialize K-CAN
  KCAN.begin();
  KCAN.setBaudRate(100000);
  KCAN.setMaxMB(16);
  KCAN.enableFIFO();
  KCAN.enableFIFOInterrupt();
  
  delay(1000);
}

void loop() {
  CAN_message_t msg;
  
  // Read PT-CAN
  if (logPTCAN) {
    while (PTCAN.read(msg)) {
      printCANMessage("PT-CAN", msg);
    }
  }
  
  // Read K-CAN
  if (logKCAN) {
    while (KCAN.read(msg)) {
      printCANMessage("K-CAN", msg);
    }
  }
}

void printCANMessage(const char* bus, const CAN_message_t &msg) {
  // Only print every PRINT_INTERVAL ms to prevent flooding
  if (millis() - lastPrint >= PRINT_INTERVAL) {
    char buffer[150];
    sprintf(buffer, "%s ID: 0x%03X LEN: %d DATA: ", bus, msg.id, msg.len);
    Serial.print(buffer);
    
    for (int i = 0; i < msg.len; i++) {
      sprintf(buffer, "%02X ", msg.buf[i]);
      Serial.print(buffer);
    }
    Serial.println();
    
    lastPrint = millis();
  }
} 