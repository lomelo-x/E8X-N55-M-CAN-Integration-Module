#ifndef M_FUNCTIONS_H
#define M_FUNCTIONS_H

#include "can_core.h"

// CAN message IDs
#define CAN_ID_M_BUTTON 0x1D9
#define CAN_ID_MDRIVE_STATUS 0x0B6
#define CAN_ID_GAUGE_CONTROL 0x6F1
#define CAN_ID_DSC_STATUS 0x398
#define CAN_ID_MHD_MAP 0x6F1  // MHD uses diagnostic messages

// M Button states
#define M_BUTTON_PRESSED 0xBF
#define M_BUTTON_RELEASED 0xFF

// Function declarations
void initialize_m_functions(void);
void handle_m_button(void);
void update_gauges(void);
void toggle_dsc_mode(void);
void set_mhd_map(uint8_t map);

// Message buffers
extern CAN_message_t mdrive_status_on_buf;
extern CAN_message_t mdrive_status_off_buf;
extern CAN_message_t speedo_needle_max_buf;
extern CAN_message_t speedo_needle_min_buf;
extern CAN_message_t tacho_needle_max_buf;
extern CAN_message_t tacho_needle_min_buf;
extern CAN_message_t dsc_on_buf;
extern CAN_message_t dsc_off_buf;
extern CAN_message_t mhd_map_buf;

#endif // M_FUNCTIONS_H 