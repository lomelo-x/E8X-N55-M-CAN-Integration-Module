#include <FlexCAN_T4.h>

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> PTCAN;  // Engine CAN
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> KCAN;   // Interior CAN

// Engine data variables
uint16_t RPM = 0;
float engine_torque = 0;
float coolant_temp = 0;
float intake_temp = 0;
float boost_pressure = 0;

void setup() {
  Serial.begin(115200);  // For debugging
  
  // Initialize PT-CAN (500kbps for BMW)
  PTCAN.begin();
  PTCAN.setBaudRate(500000);
  
  // Initialize K-CAN (100kbps for BMW comfort bus)
  KCAN.begin();
  KCAN.setBaudRate(100000);
}

void loop() {
  read_engine_data();
  update_display_data();
  delay(10);  // Small delay to prevent flooding
}

void read_engine_data() {
  CAN_message_t msg;
  
  if (PTCAN.read(msg)) {
    switch (msg.id) {
      case 0xAA:  // Engine RPM message
        RPM = ((msg.buf[0] << 8) | msg.buf[1]);
        break;
        
      case 0x0B6:  // Engine torque message
        engine_torque = ((msg.buf[2] << 4) | (msg.buf[1] >> 4)) * 0.5;
        break;
        
      // Add more cases for other engine data
    }
  }
}

void update_display_data() {
  CAN_message_t msg;
  
  // Format message for head unit display
  msg.id = 0x3F9;  // Sport display message ID
  msg.len = 8;
  
  // Pack data into message
  msg.buf[0] = RPM >> 8;
  msg.buf[1] = RPM & 0xFF;
  msg.buf[2] = (uint8_t)(engine_torque * 2);
  
  KCAN.write(msg);
} 