#include "can_core.h"
#include "m_functions.h"

// Global variables for vehicle state
static uint16_t current_rpm = 0;
static float coolant_temp = 0;
static float intake_temp = 0;
static float battery_voltage = 0;
static float turbo_pressure = 0;
static bool clutch_pressed = false;
static bool vehicle_moving = false;

void setup() {
    // Initialize serial for debugging (optional)
    Serial.begin(115200);
    
    // Initialize CAN communication
    KCAN.begin();
    KCAN.setBaudRate(100000);  // K-CAN runs at 100kbps
    
    PTCAN.begin();
    PTCAN.setBaudRate(500000); // PT-CAN runs at 500kbps
    
    // Initialize M functions
    initialize_m_functions();
    
    // Initial gauge sweep
    update_gauges();
}

void loop() {
    // Check for new CAN messages
    if (KCAN.read(k_msg)) {
        switch(k_msg.id) {
            case CAN_ID_M_BUTTON:
                handle_m_button();
                break;
                
            case CAN_ID_ENGINE_RPM:
                // Update RPM and check shift lights
                current_rpm = ((uint16_t)k_msg.buf[5] << 8) | (uint16_t)k_msg.buf[4];
                update_shift_lights(current_rpm);
                break;
                
            case CAN_ID_ENGINE_TEMP:
                // Update temperatures
                coolant_temp = k_msg.buf[0] - 48.373;  // Convert from raw value
                intake_temp = k_msg.buf[1] - 48.373;
                process_engine_diagnostics();
                break;
                
            case 0x0CE:  // Battery voltage
                battery_voltage = k_msg.buf[0] * 0.0708;
                break;
                
            case 0x0B4:  // Turbo pressure
                turbo_pressure = ((int16_t)k_msg.buf[4] - 32768) * 0.03125;
                break;
                
            case 0x1B3:  // Clutch switch
                clutch_pressed = (k_msg.buf[0] & 0x10) != 0;
                break;
                
            case 0x0A4:  // Vehicle speed
                vehicle_moving = ((uint16_t)k_msg.buf[1] << 8 | k_msg.buf[0]) > 0;
                break;
        }
        
        // Handle launch control if conditions are met
        handle_launch_control(current_rpm, clutch_pressed, vehicle_moving);
    }
    
    // Process any PT-CAN messages if needed
    if (PTCAN.read(pt_msg)) {
        // Add any PT-CAN message handling here if needed
    }
    
    delay(1); // Small delay to prevent flooding
}

void process_engine_diagnostics(void) {
    // Create custom gauge display message
    uint8_t display_data[8] = {0};
    
    // Pack the data
    display_data[0] = (uint8_t)(coolant_temp + 48.373);
    display_data[1] = (uint8_t)(intake_temp + 48.373);
    display_data[2] = (uint8_t)(battery_voltage / 0.0708);
    display_data[3] = (uint8_t)((turbo_pressure + 1) * 32);  // Convert to raw value
    
    // Create and send the message
    CAN_message_t diag_msg = make_msg_buf(CAN_ID_SPORT_DISPLAY, 8, display_data);
    kcan_write_msg(diag_msg);
} 