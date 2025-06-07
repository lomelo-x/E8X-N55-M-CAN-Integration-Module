#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h>

// Pin definitions
const int KCAN_CS_PIN = 10;  // CS pin for K-CAN MCP2515
const int PTCAN_CS_PIN = 9;  // CS pin for PT-CAN MCP2515
const int KCAN_INT_PIN = 8;  // Interrupt pin for K-CAN MCP2515
const int PTCAN_INT_PIN = 7; // Interrupt pin for PT-CAN MCP2515

// Initialize MCP2515 objects
MCP2515 kcan(KCAN_CS_PIN);   // K-CAN (100kbps)
MCP2515 ptcan(PTCAN_CS_PIN); // PT-CAN (500kbps)

// Message buffers for gauge control
struct can_frame speedo_needle_max_buf, speedo_needle_min_buf;
struct can_frame tacho_needle_max_buf, tacho_needle_min_buf;
struct can_frame oil_needle_max_buf, oil_needle_min_buf;
struct can_frame speedo_needle_release_buf, tacho_needle_release_buf, oil_needle_release_buf;

// M button state and messages
bool mButtonState = false;
struct can_frame sport_indicator_buf;
struct can_frame mdrive_control_buf;

// Constants
const uint32_t M_BUTTON_MSG_ID = 0x1D9;          // M button status message
const uint32_t SPORT_INDICATOR_MSG_ID = 0x1D2;   // Sport indicator message
const uint32_t MDRIVE_CONTROL_MSG_ID = 0x399;    // M Drive control message

void initializeGaugeMessages() {
    // Initialize speedometer messages
    speedo_needle_max_buf.can_id = 0x6F1;
    speedo_needle_max_buf.can_dlc = 8;
    uint8_t speedo_max[] = {0x60, 0x05, 0x30, 0x20, 0x06, 0x12, 0x11, 0};
    memcpy(speedo_needle_max_buf.data, speedo_max, 8);

    speedo_needle_min_buf.can_id = 0x6F1;
    speedo_needle_min_buf.can_dlc = 8;
    uint8_t speedo_min[] = {0x60, 0x05, 0x30, 0x20, 0x06, 0x00, 0x00, 0};
    memcpy(speedo_needle_min_buf.data, speedo_min, 8);

    speedo_needle_release_buf.can_id = 0x6F1;
    speedo_needle_release_buf.can_dlc = 8;
    uint8_t speedo_release[] = {0x60, 0x03, 0x30, 0x20, 0x00, 0x00, 0x00, 0};
    memcpy(speedo_needle_release_buf.data, speedo_release, 8);

    // Initialize tachometer messages
    tacho_needle_max_buf.can_id = 0x6F1;
    tacho_needle_max_buf.can_dlc = 8;
    uint8_t tacho_max[] = {0x60, 0x05, 0x30, 0x21, 0x06, 0x12, 0x3D, 0};
    memcpy(tacho_needle_max_buf.data, tacho_max, 8);

    tacho_needle_min_buf.can_id = 0x6F1;
    tacho_needle_min_buf.can_dlc = 8;
    uint8_t tacho_min[] = {0x60, 0x05, 0x30, 0x21, 0x06, 0x00, 0x00, 0};
    memcpy(tacho_needle_min_buf.data, tacho_min, 8);

    tacho_needle_release_buf.can_id = 0x6F1;
    tacho_needle_release_buf.can_dlc = 8;
    uint8_t tacho_release[] = {0x60, 0x03, 0x30, 0x21, 0x00, 0x00, 0x00, 0};
    memcpy(tacho_needle_release_buf.data, tacho_release, 8);

    // Initialize oil temperature messages
    oil_needle_max_buf.can_id = 0x6F1;
    oil_needle_max_buf.can_dlc = 8;
    uint8_t oil_max[] = {0x60, 0x05, 0x30, 0x23, 0x06, 0x07, 0x12, 0};
    memcpy(oil_needle_max_buf.data, oil_max, 8);

    oil_needle_min_buf.can_id = 0x6F1;
    oil_needle_min_buf.can_dlc = 8;
    uint8_t oil_min[] = {0x60, 0x05, 0x30, 0x23, 0x06, 0x00, 0x00, 0};
    memcpy(oil_needle_min_buf.data, oil_min, 8);

    oil_needle_release_buf.can_id = 0x6F1;
    oil_needle_release_buf.can_dlc = 8;
    uint8_t oil_release[] = {0x60, 0x03, 0x30, 0x23, 0x00, 0x00, 0x00, 0};
    memcpy(oil_needle_release_buf.data, oil_release, 8);
}

void initializeMButtonMessages() {
    // Initialize sport indicator message
    sport_indicator_buf.can_id = SPORT_INDICATOR_MSG_ID;
    sport_indicator_buf.can_dlc = 6;
    memset(sport_indicator_buf.data, 0, 8);

    // Initialize M Drive control message
    mdrive_control_buf.can_id = MDRIVE_CONTROL_MSG_ID;
    mdrive_control_buf.can_dlc = 6;
    mdrive_control_buf.data[0] = 0xE5;
    mdrive_control_buf.data[1] = 0x03;
    mdrive_control_buf.data[2] = 0x00;
    mdrive_control_buf.data[3] = 0xFF;
    mdrive_control_buf.data[4] = 0xCF; // Start with M Drive OFF
    mdrive_control_buf.data[5] = 0xFF;
}

void toggleMIndicator() {
    mButtonState = !mButtonState;
    
    if (mButtonState) {
        // Set sport mode bit
        sport_indicator_buf.data[3] = 0x04;
        // Set M Drive ON
        mdrive_control_buf.data[4] = 0xDF;
        Serial.println("M mode ON");
    } else {
        // Clear sport mode bit
        sport_indicator_buf.data[3] = 0x00;
        // Set M Drive OFF
        mdrive_control_buf.data[4] = 0xCF;
        Serial.println("M mode OFF");
    }
    
    // Send both messages
    ptcan.sendMessage(&sport_indicator_buf);
    ptcan.sendMessage(&mdrive_control_buf);
}

void performGaugeSweep() {
    Serial.println("Starting gauge sweep animation...");
    
    // Sweep to max
    kcan.sendMessage(&speedo_needle_max_buf);
    delay(100);
    kcan.sendMessage(&tacho_needle_max_buf);
    delay(100);
    kcan.sendMessage(&oil_needle_max_buf);
    delay(300);

    // Hold at max
    delay(500);

    // Sweep to min
    kcan.sendMessage(&tacho_needle_min_buf);
    delay(100);
    kcan.sendMessage(&speedo_needle_min_buf);
    delay(100);
    kcan.sendMessage(&oil_needle_min_buf);
    delay(300);

    // Hold at min
    delay(500);

    // Release control back to cluster
    kcan.sendMessage(&speedo_needle_release_buf);
    delay(100);
    kcan.sendMessage(&tacho_needle_release_buf);
    delay(100);
    kcan.sendMessage(&oil_needle_release_buf);
    
    Serial.println("Gauge sweep complete");
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) ; // Wait for serial or timeout
    Serial.println("E8X N55 M-CAN Integration Module Starting...");

    // Initialize SPI
    SPI.begin();

    // Initialize K-CAN MCP2515
    if (kcan.begin(MCP_ANY, CAN_100KBPS, MCP_8MHZ) == CAN_OK) {
        Serial.println("K-CAN Initialized Successfully!");
        kcan.setMode(MCP_NORMAL);
    } else {
        Serial.println("Error Initializing K-CAN...");
    }

    // Initialize PT-CAN MCP2515
    if (ptcan.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
        Serial.println("PT-CAN Initialized Successfully!");
        ptcan.setMode(MCP_NORMAL);
    } else {
        Serial.println("Error Initializing PT-CAN...");
    }

    // Initialize messages
    initializeGaugeMessages();
    initializeMButtonMessages();

    Serial.println("CAN buses initialized");
    
    // Perform initial gauge sweep
    performGaugeSweep();
}

void loop() {
    struct can_frame frame;
    
    // Check K-CAN messages
    if (kcan.readMessage(&frame) == CAN_OK) {
        Serial.print("K-CAN ID: 0x");
        Serial.println(frame.can_id, HEX);
        
        // Check for M button press
        if (frame.can_id == M_BUTTON_MSG_ID) {
            // Check if button is pressed (BF 7F pattern)
            if (frame.data[0] == 0xBF && frame.data[1] == 0x7F) {
                toggleMIndicator();
            }
        }
    }
    
    // Check PT-CAN messages
    if (ptcan.readMessage(&frame) == CAN_OK) {
        Serial.print("PT-CAN ID: 0x");
        Serial.println(frame.can_id, HEX);
    }

    // Periodically send M Drive status to override MSD
    static unsigned long lastMDriveUpdate = 0;
    if (millis() - lastMDriveUpdate >= 100) { // Send every 100ms
        ptcan.sendMessage(&mdrive_control_buf);
        lastMDriveUpdate = millis();
    }
}