#ifndef M_FUNCTIONS_H
#define M_FUNCTIONS_H

#include "can_core.h"

// Shift light configuration
#define START_UPSHIFT_WARN_RPM 28000    // 7000 RPM (x4)
#define MID_UPSHIFT_WARN_RPM 29600      // 7400 RPM (x4)
#define MAX_UPSHIFT_WARN_RPM 30400      // 7600 RPM (x4)
#define GONG_UPSHIFT_WARN_RPM 30800     // 7700 RPM (x4)

// Launch Control configuration
#define LC_RPM 14800                    // 3700 RPM (x4)
#define LC_RPM_MIN (LC_RPM - 800)       // 200 RPM hysteresis
#define LC_RPM_MAX (LC_RPM + 800)

// M Button states
#define M_BUTTON_PRESSED 0xBF
#define M_BUTTON_RELEASED 0xFF

// CAN message IDs
#define CAN_ID_SHIFTLIGHTS 0x206
#define CAN_ID_M_BUTTON 0x1D9
#define CAN_ID_ENGINE_TEMP 0x1D0
#define CAN_ID_ENGINE_RPM 0x0AA
#define CAN_ID_GAUGE_CONTROL 0x6F1
#define CAN_ID_MDRIVE_STATUS 0x0B6    // MDrive status message
#define CAN_ID_MDRIVE_SETTINGS 0x0DA   // MDrive settings menu
#define CAN_ID_DSC_STATUS 0x398        // DSC status/control
#define CAN_ID_SPORT_DISPLAY 0x6F1     // Sport display data

// Function declarations
void initialize_m_functions(void);
void handle_m_button(void);
void update_shift_lights(uint16_t rpm);
void update_gauges(void);
void process_engine_diagnostics(void);
void update_sport_displays(void);
void handle_launch_control(uint16_t rpm, bool clutch_pressed, bool vehicle_moving);

// MDrive functions
void toggle_mdrive_status(void);
void show_mdrive_settings(void);
bool get_mdrive_status(void);
void toggle_dsc_mode(void);
void save_mdrive_settings(void);
void load_mdrive_settings(void);

// External message buffers
extern CAN_message_t shiftlights_start_buf;
extern CAN_message_t shiftlights_mid_buildup_buf;
extern CAN_message_t shiftlights_startup_buildup_buf;
extern CAN_message_t shiftlights_max_flash_buf;
extern CAN_message_t shiftlights_off_buf;

// Gauge control messages
extern CAN_message_t speedo_needle_max_buf;
extern CAN_message_t speedo_needle_min_buf;
extern CAN_message_t tacho_needle_max_buf;
extern CAN_message_t tacho_needle_min_buf;
extern CAN_message_t oil_needle_max_buf;
extern CAN_message_t oil_needle_min_buf;

// MDrive messages
extern CAN_message_t mdrive_status_on_buf;
extern CAN_message_t mdrive_status_off_buf;
extern CAN_message_t mdrive_settings_buf;
extern CAN_message_t dsc_on_buf;
extern CAN_message_t dsc_off_buf;
extern CAN_message_t dsc_mdm_dtc_buf;

// Sport display messages
extern CAN_message_t sport_display_data_buf;

#endif // M_FUNCTIONS_H 