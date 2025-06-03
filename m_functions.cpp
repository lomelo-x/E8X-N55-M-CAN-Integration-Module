#include "m_functions.h"

// Global variables
static bool shiftlights_active = false;
static bool startup_animation_active = false;
static uint8_t ignore_shiftlights_off_counter = 0;
static bool engine_coolant_warmed_up = false;
static uint16_t var_redline_position = 0;
static uint8_t last_var_rpm_can = 0;
static bool m_button_pressed = false;
static uint8_t m_button_hold_counter = 0;
static bool mdrive_active = false;

// Message buffers
CAN_message_t shiftlights_start_buf;
CAN_message_t shiftlights_mid_buildup_buf;
CAN_message_t shiftlights_startup_buildup_buf;
CAN_message_t shiftlights_max_flash_buf;
CAN_message_t shiftlights_off_buf;
CAN_message_t speedo_needle_max_buf;
CAN_message_t speedo_needle_min_buf;
CAN_message_t tacho_needle_max_buf;
CAN_message_t tacho_needle_min_buf;
CAN_message_t oil_needle_max_buf;
CAN_message_t oil_needle_min_buf;
CAN_message_t mdrive_status_on_buf;
CAN_message_t mdrive_status_off_buf;
CAN_message_t mdrive_settings_buf;

void initialize_m_functions(void) {
    // Initialize shift light patterns
    uint8_t shiftlights_start[] = {0x86, 0x3E};
    uint8_t shiftlights_mid_buildup[] = {0xF6, 0};
    uint8_t shiftlights_startup_buildup[] = {0x86, 0};
    uint8_t shiftlights_max_flash[] = {0x0A, 0};
    uint8_t shiftlights_off[] = {0x05, 0};

    shiftlights_start_buf = make_msg_buf(CAN_ID_SHIFTLIGHTS, 2, shiftlights_start);
    shiftlights_mid_buildup_buf = make_msg_buf(CAN_ID_SHIFTLIGHTS, 2, shiftlights_mid_buildup);
    shiftlights_startup_buildup_buf = make_msg_buf(CAN_ID_SHIFTLIGHTS, 2, shiftlights_startup_buildup);
    shiftlights_max_flash_buf = make_msg_buf(CAN_ID_SHIFTLIGHTS, 2, shiftlights_max_flash);
    shiftlights_off_buf = make_msg_buf(CAN_ID_SHIFTLIGHTS, 2, shiftlights_off);

    // Initialize gauge control messages
    uint8_t speedo_max[] = {0x60, 0x05, 0x30, 0x20, 0x06, 0x12, 0x11, 0};  // 325 km/h
    uint8_t speedo_min[] = {0x60, 0x05, 0x30, 0x20, 0x06, 0x00, 0x00, 0};  // 0 km/h
    uint8_t tacho_max[] = {0x60, 0x05, 0x30, 0x21, 0x06, 0x12, 0x3D, 0};   // 8000 RPM
    uint8_t tacho_min[] = {0x60, 0x05, 0x30, 0x21, 0x06, 0x00, 0x00, 0};   // 0 RPM
    uint8_t oil_max[] = {0x60, 0x05, 0x30, 0x23, 0x06, 0x07, 0x12, 0};     // 150°C
    uint8_t oil_min[] = {0x60, 0x05, 0x30, 0x23, 0x06, 0x00, 0x00, 0};     // 0°C

    speedo_needle_max_buf = make_msg_buf(CAN_ID_GAUGE_CONTROL, 8, speedo_max);
    speedo_needle_min_buf = make_msg_buf(CAN_ID_GAUGE_CONTROL, 8, speedo_min);
    tacho_needle_max_buf = make_msg_buf(CAN_ID_GAUGE_CONTROL, 8, tacho_max);
    tacho_needle_min_buf = make_msg_buf(CAN_ID_GAUGE_CONTROL, 8, tacho_min);
    oil_needle_max_buf = make_msg_buf(CAN_ID_GAUGE_CONTROL, 8, oil_max);
    oil_needle_min_buf = make_msg_buf(CAN_ID_GAUGE_CONTROL, 8, oil_min);

    // Initialize MDrive messages
    uint8_t mdrive_on[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x97};  // MDrive ON with shiftlights
    uint8_t mdrive_off[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x87}; // MDrive OFF
    uint8_t mdrive_settings[] = {0x01, 0x00, 0x00, 0x00, 0x03, 0x89, 0x90, 0xE8}; // MDrive settings menu

    mdrive_status_on_buf = make_msg_buf(CAN_ID_MDRIVE_STATUS, 6, mdrive_on);
    mdrive_status_off_buf = make_msg_buf(CAN_ID_MDRIVE_STATUS, 6, mdrive_off);
    mdrive_settings_buf = make_msg_buf(CAN_ID_MDRIVE_SETTINGS, 8, mdrive_settings);
}

void handle_m_button(void) {
    // Read M button state from CAN message
    if (k_msg.id == CAN_ID_M_BUTTON) {
        if (k_msg.buf[0] == M_BUTTON_PRESSED && !m_button_pressed) {
            m_button_pressed = true;
            m_button_hold_counter = 0;
        }
        else if (k_msg.buf[0] == M_BUTTON_RELEASED && m_button_pressed) {
            m_button_pressed = false;
            if (m_button_hold_counter < 10) {  // Short press
                toggle_mdrive_status();
            }
            m_button_hold_counter = 0;
        }
        
        if (m_button_pressed) {
            m_button_hold_counter++;
            if (m_button_hold_counter > 10) {  // Long press
                show_mdrive_settings();
            }
        }
    }
}

void toggle_mdrive_status(void) {
    mdrive_active = !mdrive_active;
    if (mdrive_active) {
        kcan_write_msg(mdrive_status_on_buf);
    } else {
        kcan_write_msg(mdrive_status_off_buf);
    }
}

void show_mdrive_settings(void) {
    kcan_write_msg(mdrive_settings_buf);
}

bool get_mdrive_status(void) {
    return mdrive_active;
}

void update_shift_lights(uint16_t rpm) {
    if (!engine_coolant_warmed_up) {
        return;
    }

    if (rpm >= START_UPSHIFT_WARN_RPM && rpm <= MID_UPSHIFT_WARN_RPM) {
        // First yellow segment
        kcan_write_msg(shiftlights_start_buf);
        shiftlights_active = true;
    }
    else if (rpm >= MID_UPSHIFT_WARN_RPM && rpm <= MAX_UPSHIFT_WARN_RPM) {
        // Buildup yellow to red
        kcan_write_msg(shiftlights_mid_buildup_buf);
        shiftlights_active = true;
    }
    else if (rpm >= MAX_UPSHIFT_WARN_RPM) {
        // Flash red
        kcan_write_msg(shiftlights_max_flash_buf);
        shiftlights_active = true;
    }
    else if (shiftlights_active) {
        // Turn off if RPM drops
        if (ignore_shiftlights_off_counter == 0) {
            kcan_write_msg(shiftlights_off_buf);
            shiftlights_active = false;
        }
        else {
            ignore_shiftlights_off_counter--;
        }
    }
}

void update_gauges(void) {
    // Example gauge sweep animation
    kcan_write_msg(speedo_needle_min_buf);
    kcan_write_msg(tacho_needle_min_buf);
    kcan_write_msg(oil_needle_min_buf);
    delay(100);
    
    kcan_write_msg(speedo_needle_max_buf);
    kcan_write_msg(tacho_needle_max_buf);
    kcan_write_msg(oil_needle_max_buf);
    delay(100);
}

void process_engine_diagnostics(void) {
    // Process engine temperature message
    if (k_msg.id == CAN_ID_ENGINE_TEMP) {
        uint8_t coolant_temp = k_msg.buf[0] - 48;  // Celsius = value - 48
        uint8_t oil_temp = k_msg.buf[1] - 48;
        
        // Update engine status based on temperatures
        if (coolant_temp > 80) {
            engine_coolant_warmed_up = true;
        }
    }
    
    // Process engine RPM message
    if (k_msg.id == CAN_ID_ENGINE_RPM) {
        uint16_t rpm = ((k_msg.buf[2] << 8) | k_msg.buf[3]) * 4;
        update_shift_lights(rpm);
    }
} 