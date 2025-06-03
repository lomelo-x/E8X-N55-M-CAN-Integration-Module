#include "can_core.h"
#include "m_functions.h"

void setup() {
    // Initialize serial for debugging (optional)
    Serial.begin(115200);
    
    // Initialize CAN communication
    initialize_can();
    
    // Initialize M functions
    initialize_m_functions();
    
    // Initial gauge sweep
    update_gauges();
}

void loop() {
    // Check for new CAN messages
    if (KCAN.read(k_msg)) {
        // Process M button
        handle_m_button();
        
        // Process engine diagnostics (includes shift lights)
        process_engine_diagnostics();
    }
    
    // Add any additional processing here
    
    delay(1); // Small delay to prevent overwhelming the CAN bus
} 