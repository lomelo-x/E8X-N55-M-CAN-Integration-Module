// Functions that interface with the controller network(s) go here.
// KWP2000 message structure (8 bytes): {0x72, 6, 0x30, 3, 7, 0x1D, 0, 0x16};
// [0] - Controller diagnostic address. e.g FRM (0x72), JBE (0).
// [1] - ?. Sometimes represents the sequence of responses. I.e 0x10, 0x21, 0x22...
// [2] - KWP2000 SID, e.g. InputOutputControlByLocalIdentifier (0x30), StartRoutineByLocalIdentifier (0x31).
// [3] - Control target, e.g PWM-port dim value (3)
// [4] - Control type, e.g ShortTermAdjustment (7)
// [5] - Job dependent
// [6] - Job dependent
// [7] - Job dependent
// KWP jobs are reflected by the JBE across buses. I.e. sending 6F1 to KCAN will be forwarded to PTCAN too.


void cache_can_message_buffers(void) {                                                                                              // Put all static the buffers in memory during setup().
  uint8_t dsc_on[] = {0xCF, 0xE3}, dsc_mdm_dtc[] = {0xCF, 0xF3}, dsc_off[] = {0xCF, 0xE7};
  dsc_on_buf = make_msg_buf(0x398, 2, dsc_on);
  dsc_mdm_dtc_buf = make_msg_buf(0x398, 2, dsc_mdm_dtc);
  dsc_off_buf = make_msg_buf(0x398, 2, dsc_off);
  uint8_t idrive_mdrive_settings_menu_cic_a[] = {0x63, 0x10, 0xA, 0x31, 0x52, 0, 0, 6},
          idrive_mdrive_settings_menu_cic_b[] = {0x63, 0x21, 0x5C, 0, 0, 0, 0, 0},
          idrive_mdrive_settings_menu_nbt_a[] = {0x63, 0x10, 0xC, 0x31, 1, 0xA0, 0x24, 6},
          idrive_mdrive_settings_menu_nbt_b[] = {0x63, 0x21, 0xF8, 0, 0, 0, 0, 0},
          idrive_mdrive_settings_menu_nbt_c[] = {0x63, 0x22, 0, 0, 0, 0, 0, 0};
  idrive_mdrive_settings_menu_cic_a_buf = make_msg_buf(0x6F1, 8, idrive_mdrive_settings_menu_cic_a);
  idrive_mdrive_settings_menu_cic_b_buf = make_msg_buf(0x6F1, 8, idrive_mdrive_settings_menu_cic_b);
  idrive_mdrive_settings_menu_nbt_a_buf = make_msg_buf(0x6F1, 8, idrive_mdrive_settings_menu_nbt_a);
  idrive_mdrive_settings_menu_nbt_b_buf = make_msg_buf(0x6F1, 8, idrive_mdrive_settings_menu_nbt_b);
  idrive_mdrive_settings_menu_nbt_c_buf = make_msg_buf(0x6F1, 8, idrive_mdrive_settings_menu_nbt_c);

  uint8_t idrive_xview_menu_nbt_a[] = {0x63, 0x10, 0xC, 0x31, 1, 0xA0, 0x24, 7},
          idrive_xview_menu_nbt_b[] = {0x63, 0x21, 0x29, 0, 0, 0, 0, 0},
          idrive_xview_menu_nbt_c[] = {0x63, 0x22, 0, 0, 0, 0, 0, 0};
  idrive_xview_menu_nbt_a_buf = make_msg_buf(0x6F1, 8, idrive_xview_menu_nbt_a);
  idrive_xview_menu_nbt_b_buf = make_msg_buf(0x6F1, 8, idrive_xview_menu_nbt_b);
  idrive_xview_menu_nbt_c_buf = make_msg_buf(0x6F1, 8, idrive_xview_menu_nbt_c);

  uint8_t idrive_bn2000_time_12h[] = {0x1E, 0x66, 0, 1, 0, 4, 0, 0},
          idrive_bn2000_time_24h[] = {0x1E, 0x66, 0, 1, 0, 8, 0, 0},
          idrive_bn2000_date_ddmmyyyy[] = {0x1E, 0x66, 0, 1, 0, 1, 0, 0},
          idrive_bn2000_date_mmddyyyy[] = {0x1E, 0x66, 0, 1, 0, 2, 0, 0},
          idrive_bn2000_consumption_l100km[] = {0x1E, 0x66, 0, 1, 0, 0, 1, 0},
          idrive_bn2000_consumption_mpguk[] = {0x1E, 0x66, 0, 1, 0, 0, 2, 0},
          idrive_bn2000_consumption_mpgus[] = {0x1E, 0x66, 0, 1, 0, 0, 3, 0},
          idrive_bn2000_consumption_kml[] = {0x1E, 0x66, 0, 1, 0, 0, 4, 0},
          idrive_bn2000_distance_km[] = {0x1E, 0x66, 0, 1, 0, 0, 0x40, 0},
          idrive_bn2000_distance_mi[] = {0x1E, 0x66, 0, 1, 0, 0, 0x80, 0},
          idrive_bn2000_pressure_bar[] = {0x1E, 0x66, 0, 1, 0, 0, 0, 1},
          idrive_bn2000_pressure_kpa[] = {0x1E, 0x66, 0, 1, 0, 0, 0, 2},
          idrive_bn2000_pressure_psi[] = {0x1E, 0x66, 0, 1, 0, 0, 0, 3},
          idrive_bn2000_temperature_c[] = {0x1E, 0x66, 0, 1, 0, 0x10, 0, 0},
          idrive_bn2000_temperature_f[] = {0x1E, 0x66, 0, 1, 0, 0x20, 0, 0},
          idrive_bn2000_hba_on[] = {0x1E, 0x4A, 0, 1, 0x20, 0, 0, 0},
          idrive_bn2000_hba_off[] = {0x1E, 0x4A, 0, 1, 0x10, 0, 0, 0},
          idrive_bn2000_indicator_single[] = {0x1E, 0x48, 0, 1, 0x10, 0xFF, 0, 0},
          idrive_bn2000_indicator_triple[] = {0x1E, 0x48, 0, 1, 0x20, 0xFF, 0, 0},
          idrive_bn2000_drl_on[] = {0x1E, 0x48, 0, 1, 0, 0xFF, 2, 0},
          idrive_bn2000_drl_off[] = {0x1E, 0x48, 0, 1, 0, 0xFF, 1, 0};
  idrive_bn2000_time_12h_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_time_12h);
  idrive_bn2000_time_24h_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_time_24h);
  idrive_bn2000_date_ddmmyyyy_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_date_ddmmyyyy);
  idrive_bn2000_date_mmddyyyy_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_date_mmddyyyy);
  idrive_bn2000_consumption_l100km_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_consumption_l100km);
  idrive_bn2000_consumption_mpgus_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_consumption_mpgus);
  idrive_bn2000_consumption_mpguk_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_consumption_mpguk);
  idrive_bn2000_consumption_kml_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_consumption_kml);
  idrive_bn2000_distance_km_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_distance_km);
  idrive_bn2000_distance_mi_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_distance_mi);
  idrive_bn2000_pressure_bar_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_pressure_bar);
  idrive_bn2000_pressure_kpa_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_pressure_kpa);
  idrive_bn2000_pressure_psi_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_pressure_psi);
  idrive_bn2000_temperature_c_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_temperature_c);
  idrive_bn2000_temperature_f_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_temperature_f);
  idrive_bn2000_hba_on_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_hba_on);
  idrive_bn2000_hba_off_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_hba_off);
  idrive_bn2000_indicator_single_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_indicator_single);
  idrive_bn2000_indicator_triple_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_indicator_triple);
  idrive_bn2000_drl_on_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_drl_on);
  idrive_bn2000_drl_off_buf = make_msg_buf(0x5E2, 8, idrive_bn2000_drl_off);

  uint8_t set_warning_15kph_off[] = {0x3E, 0xF, 0xF0, 0xFE, 0xFF, 0xEE},                                                                // OFF, 15kph.
          set_warning_15kph_on[] = {0x7E, 0xF, 0xF0, 0xFE, 0xFF, 0xEE};
  set_warning_15kph_off_buf = make_msg_buf(0x2B8, 6, set_warning_15kph_off);
  set_warning_15kph_on_buf = make_msg_buf(0x2B8, 6, set_warning_15kph_on);

  uint8_t gws_sport_on[] = {0, 0, 0, 4, 0, 0, 0},
          gws_sport_off[] = {0, 0, 0, 0, 0, 0, 0};
  gws_sport_on_buf = make_msg_buf(0x1D2, 6, gws_sport_on);
  gws_sport_off_buf = make_msg_buf(0x1D2, 6, gws_sport_off);

  uint8_t cc_single_gong[] = {0xF1, 0xFF},
          cc_double_gong[] = {0xF2, 0xFF},
          cc_triple_gong[] = {0xF3, 0xFF},
          idrive_horn_sound[] = {0xFD,0xFF};
  cc_single_gong_buf = make_msg_buf(0x205, 2, cc_single_gong);
  cc_double_gong_buf = make_msg_buf(0x205, 2, cc_double_gong);
  cc_triple_gong_buf = make_msg_buf(0x205, 2, cc_triple_gong);
  idrive_horn_sound_buf = make_msg_buf(0x205, 2, idrive_horn_sound);

  #if F_NBTE
    uint8_t idrive_button_sound[] = {0x63, 5, 0x31, 1, 0xA0, 0, 0x13, 0},
            idrive_beep_sound[] = {0x63, 5, 0x31, 1, 0xA0, 0, 0x10, 0};
    idrive_button_sound_buf = make_msg_buf(0x6F1, 8, idrive_button_sound);
    idrive_beep_sound_buf = make_msg_buf(0x6F1, 8, idrive_beep_sound);
  #else
    uint8_t idrive_button_sound[] = {0x63, 3, 0x31, 0x21, 7},
            idrive_beep_sound[] = {0x63, 3, 0x31, 0x21, 0x12};
    idrive_button_sound_buf = make_msg_buf(0x6F1, 5, idrive_button_sound);
    idrive_beep_sound_buf = make_msg_buf(0x6F1, 5, idrive_beep_sound);
  #endif

  uint8_t clear_hs_kwp_dme[] = {0x12, 2, 0x31, 3, 0, 0, 0, 0};
  clear_hs_kwp_dme_buf = make_msg_buf(0x6F1, 8, clear_hs_kwp_dme);

  uint8_t dme_boost_request_a[] = {0x12, 3, 0x30, 0x19, 1, 0, 0, 0},
          dme_boost_request_b[] = {0x12, 0x30, 0, 0, 0, 0, 0, 0},
          dme_request_consumers_off[] = {0x1F, 0, 0xFF, 0x7F, 0, 8, 0xF2};			                                                    // CTR_PCOS - 2, ST_ENERG_PWMG - 2.
  dme_boost_request_a_buf = make_msg_buf(0x6F1, 8, dme_boost_request_a);
  dme_boost_request_b_buf = make_msg_buf(0x6F1, 8, dme_boost_request_b);
  dme_request_consumers_off_buf = make_msg_buf(0x3B3, 7, dme_request_consumers_off);

  uint8_t fzm_wake[] = {0xE1, 0xD5, 0xFF, 0xFF, 0xFF},
          fzm_sleep[] = {0xE1, 0xD6, 0xFF, 0xFF, 0xFF};
  fzm_wake_buf = make_msg_buf(0x3A5, 5, fzm_wake);                                                                                  // 6MC2DL0B pg. 1531.
  fzm_sleep_buf = make_msg_buf(0x3A5, 5, fzm_sleep);

  uint8_t ccc_zbe_wake[] = {0xFE, 3, 0, 0, 0, 0, 0, 0};
  ccc_zbe_wake_buf = make_msg_buf(0x1AA, 8, ccc_zbe_wake);

  uint8_t nbt_vin_request[] = {0x63, 3, 0x22, 0xF1, 0x90, 0, 0, 0x40};
  nbt_vin_request_buf = make_msg_buf(0x6F1, 8, nbt_vin_request);

  uint8_t faceplate_a1_released[] = {0, 0xFF},
          faceplate_power_mute[] = {4, 0xFF},
          faceplate_eject[] = {1, 0xFF},
          faceplate_a2_released[] = {0, 0},
          faceplate_button1_hover[] = {1, 0},
          faceplate_button1_press[] = {2, 0},
          faceplate_button2_hover[] = {4, 0},
          faceplate_button2_press[] = {8, 0},
          faceplate_button3_hover[] = {0x10, 0},
          faceplate_button3_press[] = {0x20, 0},
          faceplate_button4_hover[] = {0x40, 0},
          faceplate_button4_press[] = {0x80, 0},
          faceplate_button5_hover[] = {0, 1},
          faceplate_button5_press[] = {0, 2},
          faceplate_button6_hover[] = {0, 4},
          faceplate_button6_press[] = {0, 8},
          faceplate_button7_hover[] = {0, 0x10},
          faceplate_button7_press[] = {0, 0x20},
          faceplate_button8_hover[] = {0, 0x40},
          faceplate_button8_press[] = {0, 0x80},
          faceplate_a3_released[] = {0xFC, 0xFF},
          faceplate_seek_left[] = {0xFD, 0xFF},
          faceplate_seek_right[] = {0xFE, 0xFF},
          faceplate_f1_released[] = {0, 0xFC},
          faceplate_volume_decrease[] = {1, 0xFE},
          faceplate_volume_increase[] = {1, 0xFD};
  faceplate_a1_released_buf = make_msg_buf(0xA1, 2, faceplate_a1_released);
  faceplate_power_mute_buf = make_msg_buf(0xA1, 2, faceplate_power_mute);
  faceplate_eject_buf = make_msg_buf(0xA1, 2, faceplate_eject);
  faceplate_a2_released_buf = make_msg_buf(0xA2, 2, faceplate_a2_released);
  faceplate_button1_hover_buf = make_msg_buf(0xA2, 2, faceplate_button1_hover);
  faceplate_button1_press_buf = make_msg_buf(0xA2, 2, faceplate_button1_press);
  faceplate_button2_hover_buf = make_msg_buf(0xA2, 2, faceplate_button2_hover);
  faceplate_button2_press_buf = make_msg_buf(0xA2, 2, faceplate_button2_press);
  faceplate_button3_hover_buf = make_msg_buf(0xA2, 2, faceplate_button3_hover);
  faceplate_button3_press_buf = make_msg_buf(0xA2, 2, faceplate_button3_press);
  faceplate_button4_hover_buf = make_msg_buf(0xA2, 2, faceplate_button4_hover);
  faceplate_button4_press_buf = make_msg_buf(0xA2, 2, faceplate_button4_press);
  faceplate_button5_hover_buf = make_msg_buf(0xA2, 2, faceplate_button5_hover);
  faceplate_button5_press_buf = make_msg_buf(0xA2, 2, faceplate_button5_press);
  faceplate_button6_hover_buf = make_msg_buf(0xA2, 2, faceplate_button6_hover);
  faceplate_button6_press_buf = make_msg_buf(0xA2, 2, faceplate_button6_press);
  faceplate_button7_hover_buf = make_msg_buf(0xA2, 2, faceplate_button7_hover);
  faceplate_button7_press_buf = make_msg_buf(0xA2, 2, faceplate_button7_press);
  faceplate_button8_hover_buf = make_msg_buf(0xA2, 2, faceplate_button8_hover);
  faceplate_button8_press_buf = make_msg_buf(0xA2, 2, faceplate_button8_press);
  faceplate_a3_released_buf = make_msg_buf(0xA3, 2, faceplate_a3_released);
  faceplate_seek_left_buf = make_msg_buf(0xA3, 2, faceplate_seek_left);
  faceplate_seek_right_buf = make_msg_buf(0xA3, 2, faceplate_seek_right);
  faceplate_f1_released_buf = make_msg_buf(0xF1, 2, faceplate_f1_released);
  faceplate_volume_decrease_buf = make_msg_buf(0xF1, 2, faceplate_volume_decrease);
  faceplate_volume_increase_buf = make_msg_buf(0xF1, 2, faceplate_volume_increase);

  uint8_t ftm_indicator_flash[] = {0x40, 0x50, 1, 0x69, 0xFF, 0xFF, 0xFF, 0xFF},
          ftm_indicator_off[] = {0x40, 0x50, 1, 0, 0xFF, 0xFF, 0xFF, 0xFF};
  ftm_indicator_flash_buf = make_msg_buf(0x5A0, 8, ftm_indicator_flash);
  ftm_indicator_off_buf = make_msg_buf(0x5A0, 8, ftm_indicator_off);

  uint8_t frm_ckm_ahl_komfort[] = {0, 4}, frm_ckm_ahl_sport[] = {0, 0xA};
  frm_ckm_ahl_komfort_buf = make_msg_buf(0x3F0, 2, frm_ckm_ahl_komfort);
  frm_ckm_ahl_sport_buf = make_msg_buf(0x3F0, 2, frm_ckm_ahl_sport);

  uint8_t frm_toggle_fold_mirror_a[] = {0x72, 0x10, 7, 0x30, 0x10, 7, 1, 5},
          frm_toggle_fold_mirror_b[] = {0x72, 0x21, 0, 1, 0, 0, 0, 0},
          frm_mirror_status_request_a[] = {0x72, 3, 0x30, 0x16, 1, 0, 0, 0},
          frm_mirror_status_request_b[] = {0x72, 0x30, 0, 0, 0, 0, 0, 0};
  frm_toggle_fold_mirror_a_buf = make_msg_buf(0x6F1, 8, frm_toggle_fold_mirror_a);
  frm_toggle_fold_mirror_b_buf = make_msg_buf(0x6F1, 8, frm_toggle_fold_mirror_b);
  frm_mirror_status_request_a_buf = make_msg_buf(0x6F1, 8, frm_mirror_status_request_a);
  frm_mirror_status_request_b_buf = make_msg_buf(0x6F1, 8, frm_mirror_status_request_b);

  uint8_t frm_mirror_undim[] = {0x72, 5, 0x30, 0x11, 7, 0, 0x90, 0};
  frm_mirror_undim_buf = make_msg_buf(0x6F1, 8, frm_mirror_undim);
  
  uint8_t flash_hazards_single[] = {0, 0xF1},
          flash_hazards_double[] = {0, 0xF2},
          flash_hazards_single_long[] = {0, 0xF3},
          flash_hazards_angel_eyes[] = {5, 0},
          flash_hazards_angel_eyes_xenons[] = {7, 0},
          stop_flashing_lights[] = {0, 0},
          alarm_beep_6x[] = {0x41, 3, 0x31, 0xFD, 0};
  flash_hazards_single_buf = make_msg_buf(0x2B4, 2, flash_hazards_single);                                                          // Minimum time between flash messages is 1.3s.
  flash_hazards_double_buf = make_msg_buf(0x2B4, 2, flash_hazards_double);
  flash_hazards_single_long_buf = make_msg_buf(0x2B4, 2, flash_hazards_single_long);
  flash_hazards_angel_eyes_buf = make_msg_buf(0x2B4, 2, flash_hazards_angel_eyes);
  flash_hazards_angel_eyes_xenons_buf = make_msg_buf(0x2B4, 2, flash_hazards_angel_eyes_xenons);
  stop_flashing_lights_buf = make_msg_buf(0x2B4, 2, stop_flashing_lights);
  alarm_beep_6x_buf = make_msg_buf(0x6F1, 5, alarm_beep_6x);

  uint8_t alarm_siren_on[] = {0x41, 3, 0x31, 4, 2, 0, 0, 0},
          alarm_siren_return_control[] = {0x41, 3, 0x31, 4, 3, 0, 0, 0},
          alarm_led_on[] = {0x41, 4, 0x30, 2, 7, 1, 0, 0},
          alarm_led_return_control[] = {0x41, 3, 0x30, 2, 0, 0, 0, 0};
  alarm_siren_on_buf = make_msg_buf(0x6F1, 8, alarm_siren_on);
  alarm_siren_return_control_buf = make_msg_buf(0x6F1, 8, alarm_siren_return_control);
  alarm_led_on_buf = make_msg_buf(0x6F1, 8, alarm_led_on);
  alarm_led_return_control_buf = make_msg_buf(0x6F1, 8, alarm_led_return_control);

  uint8_t ekp_pwm_off[] = {0x17, 4, 0x30, 6, 4, 0, 0, 0},
          ekp_return_to_normal[] = {0x17, 2, 0x30, 0, 0, 0, 0, 0},
          key_cc_on[] = {0x40, 0x26, 0, 0x39, 0xFF, 0xFF, 0xFF, 0xFF},
          key_cc_off[] = {0x40, 0x26, 0, 0x30, 0xFF, 0xFF, 0xFF, 0xFF},
          start_cc_on[] = {0x40, 0x2F, 1, 0x39, 0xFF, 0xFF, 0xFF, 0xFF},
          start_cc_off[] = {0x40, 0x2F, 1, 0x30, 0xFF, 0xFF, 0xFF, 0xFF};
  key_cc_on_buf = make_msg_buf(0x5C0, 8, key_cc_on);
  key_cc_off_buf = make_msg_buf(0x5C0, 8, key_cc_off);
  start_cc_on_buf = make_msg_buf(0x5C0, 8, start_cc_on);
  start_cc_off_buf = make_msg_buf(0x5C0, 8, start_cc_off);
  ekp_pwm_off_buf = make_msg_buf(0x6F1, 8, ekp_pwm_off);
  ekp_return_to_normal_buf = make_msg_buf(0x6F1, 8, ekp_return_to_normal);

  uint8_t dr_seat_move_back[] = {0x6D, 6, 0x30, 0x10, 6, 8, 2, 0xA};
  dr_seat_move_back_buf = make_msg_buf(0x6F1, 8, dr_seat_move_back);

  uint8_t front_left_fog_on_a[] = {0x72, 6, 0x30, 3, 7, 6, 0, 8},                                                                   // Soft on/off buffers.
          front_left_fog_on_b[] = {0x72, 6, 0x30, 3, 7, 6, 0, 0x16},
          front_left_fog_on_c[] = {0x72, 6, 0x30, 3, 7, 6, 0, 0x32},
          front_left_fog_on_d[] = {0x72, 6, 0x30, 3, 7, 6, 0, 0x64},
          front_left_fog_on_a_softer[] = {0x72, 6, 0x30, 3, 7, 6, 0, 8},
          front_left_fog_on_b_softer[] = {0x72, 6, 0x30, 3, 7, 6, 0, 0x16},
          front_left_fog_on_c_softer[] = {0x72, 6, 0x30, 3, 7, 6, 0, 0x24},
          front_left_fog_on_d_softer[] = {0x72, 6, 0x30, 3, 7, 6, 0, 0x32},
          front_left_fog_on_e_softer[] = {0x72, 6, 0x30, 3, 7, 6, 0, 0x40},
          front_left_fog_on_f_softer[] = {0x72, 6, 0x30, 3, 7, 6, 0, 0x48},
          front_left_fog_on_g_softer[] = {0x72, 6, 0x30, 3, 7, 6, 0, 0x56},
          front_left_fog_on_h_softer[] = {0x72, 6, 0x30, 3, 7, 6, 0, 0x64},
          front_left_fog_off[] = {0x72, 6, 0x30, 3, 7, 6, 0, 0},
          front_right_fog_on_a[] = {0x72, 6, 0x30, 3, 7, 7, 0, 8},
          front_right_fog_on_b[] = {0x72, 6, 0x30, 3, 7, 7, 0, 0x16},
          front_right_fog_on_c[] = {0x72, 6, 0x30, 3, 7, 7, 0, 0x32},
          front_right_fog_on_d[] = {0x72, 6, 0x30, 3, 7, 7, 0, 0x64},
          front_right_fog_on_a_softer[] = {0x72, 6, 0x30, 3, 7, 7, 0, 8},
          front_right_fog_on_b_softer[] = {0x72, 6, 0x30, 3, 7, 7, 0, 0x16},
          front_right_fog_on_c_softer[] = {0x72, 6, 0x30, 3, 7, 7, 0, 0x24},
          front_right_fog_on_d_softer[] = {0x72, 6, 0x30, 3, 7, 7, 0, 0x32},
          front_right_fog_on_e_softer[] = {0x72, 6, 0x30, 3, 7, 7, 0, 0x40},
          front_right_fog_on_f_softer[] = {0x72, 6, 0x30, 3, 7, 7, 0, 0x48},
          front_right_fog_on_g_softer[] = {0x72, 6, 0x30, 3, 7, 7, 0, 0x56},
          front_right_fog_on_h_softer[] = {0x72, 6, 0x30, 3, 7, 7, 0, 0x64},
          front_right_fog_off[] = {0x72, 6, 0x30, 3, 7, 7, 0, 0},
          front_fogs_all_off[] = {0x72, 6, 0x30, 0x29, 7, 0, 1, 2};
  front_left_fog_on_a_buf = make_msg_buf(0x6F1, 8, front_left_fog_on_a);
  front_left_fog_on_b_buf = make_msg_buf(0x6F1, 8, front_left_fog_on_b);
  front_left_fog_on_c_buf = make_msg_buf(0x6F1, 8, front_left_fog_on_c);
  front_left_fog_on_d_buf = make_msg_buf(0x6F1, 8, front_left_fog_on_d);
  front_left_fog_on_a_softer_buf = make_msg_buf(0x6F1, 8, front_left_fog_on_a_softer);
  front_left_fog_on_b_softer_buf = make_msg_buf(0x6F1, 8, front_left_fog_on_b_softer);
  front_left_fog_on_c_softer_buf = make_msg_buf(0x6F1, 8, front_left_fog_on_c_softer);
  front_left_fog_on_d_softer_buf = make_msg_buf(0x6F1, 8, front_left_fog_on_d_softer);
  front_left_fog_on_e_softer_buf = make_msg_buf(0x6F1, 8, front_left_fog_on_e_softer);
  front_left_fog_on_f_softer_buf = make_msg_buf(0x6F1, 8, front_left_fog_on_f_softer);
  front_left_fog_on_g_softer_buf = make_msg_buf(0x6F1, 8, front_left_fog_on_g_softer);
  front_left_fog_on_h_softer_buf = make_msg_buf(0x6F1, 8, front_left_fog_on_h_softer);
  front_left_fog_off_buf = make_msg_buf(0x6F1, 8, front_left_fog_off);
  front_right_fog_on_a_buf = make_msg_buf(0x6F1, 8, front_right_fog_on_a);
  front_right_fog_on_b_buf = make_msg_buf(0x6F1, 8, front_right_fog_on_b);
  front_right_fog_on_c_buf = make_msg_buf(0x6F1, 8, front_right_fog_on_c);
  front_right_fog_on_d_buf = make_msg_buf(0x6F1, 8, front_right_fog_on_d);
  front_right_fog_on_a_softer_buf = make_msg_buf(0x6F1, 8, front_right_fog_on_a_softer);
  front_right_fog_on_b_softer_buf = make_msg_buf(0x6F1, 8, front_right_fog_on_b_softer);
  front_right_fog_on_c_softer_buf = make_msg_buf(0x6F1, 8, front_right_fog_on_c_softer);
  front_right_fog_on_d_softer_buf = make_msg_buf(0x6F1, 8, front_right_fog_on_d_softer);
  front_right_fog_on_e_softer_buf = make_msg_buf(0x6F1, 8, front_right_fog_on_e_softer);
  front_right_fog_on_f_softer_buf = make_msg_buf(0x6F1, 8, front_right_fog_on_f_softer);
  front_right_fog_on_g_softer_buf = make_msg_buf(0x6F1, 8, front_right_fog_on_g_softer);
  front_right_fog_on_h_softer_buf = make_msg_buf(0x6F1, 8, front_right_fog_on_h_softer);
  front_right_fog_off_buf = make_msg_buf(0x6F1, 8, front_right_fog_off);
  front_fogs_all_off_buf = make_msg_buf(0x6F1, 8, front_fogs_all_off);                                                              // This job only works with ignition ON.

  uint8_t frm_ahl_status_request[] = {0x72, 3, 0x30, 0x28, 1, 0, 0, 0};
  frm_ahl_flc_status_request_buf = make_msg_buf(0x6F1, 8, frm_ahl_status_request);

  uint8_t left_drl_off[] = {0x72, 6, 0x30, 3, 7, 0x1D, 0, 0},
          left_drl_dim[] = {0x72, 6, 0x30, 3, 7, 0x1D, 0, 0x16},
          left_drl_bright[] = {0x72, 6, 0x30, 3, 7, 0x1D, 0, 0x64},
          right_drl_off[] = {0x72, 6, 0x30, 3, 7, 0x1E, 0, 0},
          right_drl_dim[] = {0x72, 6, 0x30, 3, 7, 0x1E, 0, 0x16},
          right_drl_bright[] = {0x72, 6, 0x30, 3, 7, 0x1E, 0, 0x64};
  left_drl_dim_off = make_msg_buf(0x6F1, 8, left_drl_off);
  left_drl_dim_buf = make_msg_buf(0x6F1, 8, left_drl_dim);
  left_drl_bright_buf = make_msg_buf(0x6F1, 8, left_drl_bright);
  right_drl_dim_off = make_msg_buf(0x6F1, 8, right_drl_off);
  right_drl_dim_buf = make_msg_buf(0x6F1, 8, right_drl_dim);
  right_drl_bright_buf = make_msg_buf(0x6F1, 8, right_drl_bright);

  uint8_t f_kombi_network_management[] = {0, 0, 0, 0, 0x57, 0x2F, 0, 0x60};                                                         // AUTOSAR Network management KOMBI - F-series.
  f_kombi_network_management_buf = make_msg_buf(0x560, 8, f_kombi_network_management);

  uint8_t f_zgw_network_management[] = {0, 0, 0, 0, 0xFF, 0xFF, 0, 0x10};                                                           // AUTOSAR Network management ZGW - F-series.
  f_zgw_network_management_buf = make_msg_buf(0x510, 8, f_zgw_network_management);

  uint8_t sine_pitch_angle_request_a[] = {0x50, 2, 0x21, 5},
          sine_pitch_angle_request_b[] = {0x50, 0x30, 0, 2, 0xFF, 0xFF, 0xFF, 0xFF},
          sine_roll_angle_request_a[] = {0x50, 2, 0x21, 4},
          sine_roll_angle_request_b[] = {0x50, 0x30, 0, 2, 0xFF, 0xFF, 0xFF, 0xFF};
  sine_pitch_angle_request_a_buf = make_msg_buf(0x6F1, 4, sine_pitch_angle_request_a);
  sine_pitch_angle_request_b_buf = make_msg_buf(0x6F1, 8, sine_pitch_angle_request_b);
  sine_roll_angle_request_a_buf = make_msg_buf(0x6F1, 4, sine_roll_angle_request_a);
  sine_roll_angle_request_b_buf = make_msg_buf(0x6F1, 8, sine_roll_angle_request_b);

  uint8_t lc_cc_on[] = {0x40, 0xBE, 1, 0x39, 0xFF, 0xFF, 0xFF, 0xFF},
          lc_cc_off[] = {0x40, 0xBE, 1, 0x30, 0xFF, 0xFF, 0xFF, 0xFF};
  lc_cc_on_buf = make_msg_buf(0x598, 8, lc_cc_on);
  lc_cc_off_buf = make_msg_buf(0x598, 8, lc_cc_off);

  uint8_t generic_button_pressed[] = {0xFD, 0xFF}, generic_button_released[] = {0xFC, 0xFF};
  seat_heating_button_pressed_dr_buf = make_msg_buf(0x1E7, 2, generic_button_pressed);
  seat_heating_button_released_dr_buf = make_msg_buf(0x1E7, 2, generic_button_released);
  seat_heating_button_pressed_pas_buf = make_msg_buf(0x1E8, 2, generic_button_pressed);
  seat_heating_button_released_pas_buf = make_msg_buf(0x1E8, 2, generic_button_released);

  uint8_t shiftlights_start[] = {0x86, 0x3E}, 
          shiftlights_mid_buildup[] = {0xF6, 0},
  #if NEEDLE_SWEEP
          shiftlights_startup_buildup[] = {0x86, 0},
  #else
          shiftlights_startup_buildup[] = {0x56, 0},                                                                                // Faster sequential buildup. First 8 bits: 0-0xF (0xF - slowest).
  #endif
          shiftlights_max_flash[] = {0xA, 0},
          shiftlights_off[] = {5, 0};
  shiftlights_start_buf = make_msg_buf(0x206, 2, shiftlights_start);
  shiftlights_mid_buildup_buf = make_msg_buf(0x206, 2, shiftlights_mid_buildup);
  shiftlights_startup_buildup_buf = make_msg_buf(0x206, 2, shiftlights_startup_buildup);
  shiftlights_max_flash_buf = make_msg_buf(0x206, 2, shiftlights_max_flash);
  shiftlights_off_buf = make_msg_buf(0x206, 2, shiftlights_off);

  uint8_t speedo_needle_max[] = {0x60, 5, 0x30, 0x20, 6, 0x12, 0x11, 0},                                                            // Set to 325 KM/h
          speedo_needle_min[] = {0x60, 5, 0x30, 0x20, 6, 0, 0, 0},                                                                  // Set to 0
          speedo_needle_release[] = {0x60, 3, 0x30, 0x20, 0, 0, 0, 0},
          tacho_needle_max[] = {0x60, 5, 0x30, 0x21, 6, 0x12, 0x3D, 0},                                                             // Set to 8000 RPM
          tacho_needle_min[] = {0x60, 5, 0x30, 0x21, 6, 0, 0, 0},                                                                   // Set to 0
          tacho_needle_release[] = {0x60, 3, 0x30, 0x21, 0, 0, 0, 0},
          fuel_needle_max[] = {0x60, 5, 0x30, 0x22, 6, 7, 0x4E, 0},                                                                 // Set to 100%
          fuel_needle_min[] = {0x60, 5, 0x30, 0x22, 6, 0, 0, 0},                                                                    // Set to 0%
          fuel_needle_release[] = {0x60, 3, 0x30, 0x22, 0, 0, 0, 0},
          oil_needle_max[] = {0x60, 5, 0x30, 0x23, 6, 7, 0x12, 0},                                                                  // Set to 150 C
          oil_needle_min[] = {0x60, 5, 0x30, 0x23, 6, 0, 0, 0},                                                                     // Set to 0 C
          oil_needle_release[] = {0x60, 3, 0x30, 0x23, 0, 0, 0, 0};
  speedo_needle_max_buf = make_msg_buf(0x6F1, 8, speedo_needle_max);
  speedo_needle_min_buf = make_msg_buf(0x6F1, 8, speedo_needle_min);
  speedo_needle_release_buf = make_msg_buf(0x6F1, 8, speedo_needle_release);
  tacho_needle_max_buf = make_msg_buf(0x6F1, 8, tacho_needle_max);
  tacho_needle_min_buf = make_msg_buf(0x6F1, 8, tacho_needle_min);
  tacho_needle_release_buf = make_msg_buf(0x6F1, 8, tacho_needle_release);
  fuel_needle_max_buf = make_msg_buf(0x6F1, 8, fuel_needle_max);
  fuel_needle_min_buf = make_msg_buf(0x6F1, 8, fuel_needle_min);
  fuel_needle_release_buf = make_msg_buf(0x6F1, 8, fuel_needle_release);
  oil_needle_max_buf = make_msg_buf(0x6F1, 8, oil_needle_max);
  oil_needle_min_buf = make_msg_buf(0x6F1, 8, oil_needle_min);
  oil_needle_release_buf = make_msg_buf(0x6F1, 8, oil_needle_release);

  #if F_NBTE
    uint8_t vol_request[] = {0x63, 5, 0x31, 1, 0xA0, 0x39, 0};
    vol_request_buf = make_msg_buf(0x6F1, 7, vol_request);
    uint8_t custom_cc_dismiss[] = {0x46, 3, 0x50, 0xF0, 0, 0, 0, 0},
            custom_cc_clear[] = {0x46, 3, 0x70, 0xF0, 0, 0, 0, 0};
    custom_cc_dismiss_buf = make_msg_buf(0x338, 8, custom_cc_dismiss);
    custom_cc_clear_buf = make_msg_buf(0x338, 8, custom_cc_clear);
    uint8_t hood_open_hot_cc_dialog[] = {0xE1, 2, 0x32, 0xF0, 0, 0xFE, 0xFE, 0xFE},
          hood_open_hot_cc_dialog_clear[] = {0xE1, 2, 0x70, 0xF0, 0, 0xFE, 0xFE, 0xFE};
    hood_open_hot_cc_dialog_buf = make_msg_buf(0x338, 8, hood_open_hot_cc_dialog);
    hood_open_hot_cc_dialog_clear_buf = make_msg_buf(0x338, 8, hood_open_hot_cc_dialog_clear);
  #else
    uint8_t vol_request[] = {0x63, 3, 0x31, 0x24, 0, 0, 0, 0};
    vol_request_buf = make_msg_buf(0x6F1, 8, vol_request);
  #endif

  uint8_t hdc_cc_activated_on[] = {0x40, 0x4B, 1, 0x1D, 0xFF, 0xFF, 0xFF, 0xFF},
          hdc_cc_unavailable_on[] = {0x40, 0x4D, 1, 0x1D, 0xFF, 0xFF, 0xFF, 0xFF},
          hdc_cc_deactivated_on[] = {0x40, 0x4C, 1, 0x1D, 0xFF, 0xFF, 0xFF, 0xFF},
          hdc_cc_activated_off[] = {0x40, 0x4B, 1, 0, 0xFF, 0xFF, 0xFF, 0xFF},
          hdc_cc_unavailable_off[] = {0x40, 0x4D, 1, 0, 0xFF, 0xFF, 0xFF, 0xFF},
          hdc_cc_deactivated_off[] = {0x40, 0x4C, 1, 0, 0xFF, 0xFF, 0xFF, 0xFF};
  hdc_cc_activated_on_buf = make_msg_buf(0x5A9, 8, hdc_cc_activated_on);
  hdc_cc_unavailable_on_buf = make_msg_buf(0x5A9, 8, hdc_cc_unavailable_on);
  hdc_cc_deactivated_on_buf = make_msg_buf(0x5A9, 8, hdc_cc_deactivated_on);
  hdc_cc_activated_off_buf = make_msg_buf(0x5A9, 8, hdc_cc_activated_off);
  hdc_cc_unavailable_off_buf = make_msg_buf(0x5A9, 8, hdc_cc_unavailable_off);
  hdc_cc_deactivated_off_buf = make_msg_buf(0x5A9, 8, hdc_cc_deactivated_off);

  uint8_t msa_deactivated_cc_on[] = {0x40, 0xC2, 1, 0x39, 0xFF, 0xFF, 0xFF, 0xFF},
          msa_deactivated_cc_off[] = {0x40, 0xC2, 1, 0x30, 0xFF, 0xFF, 0xFF, 0xFF};
  msa_deactivated_cc_on_buf = make_msg_buf(0x592, 8, msa_deactivated_cc_on);
  msa_deactivated_cc_off_buf = make_msg_buf(0x592, 8, msa_deactivated_cc_off);

  uint8_t camera_off[] = {0xA1, 0xFF}, camera_on[] = {0xA5, 0xFF}, 
          camera_inactive[] = {0x81, 0xFF};
  camera_off_buf = make_msg_buf(0x3AE, 2, camera_off);
  camera_on_buf = make_msg_buf(0x3AE, 2, camera_on);
  camera_inactive_buf = make_msg_buf(0x3AE, 2, camera_inactive);
  pdc_button_presssed_buf = make_msg_buf(0x317, 2, generic_button_pressed);
  pdc_button_released_buf = make_msg_buf(0x317, 2, generic_button_released);

  uint8_t msa_fake_status[] = {0xFF, 0xD2, 0xF1};
  msa_fake_status_buf = make_msg_buf(0x308, 3, msa_fake_status);

  uint8_t radon_asd[] = {0x3F, 4, 0x31, 0xB8, 0xB, 1, 0, 0},
          mute_asd[] = {0x3F, 5, 0x31, 0xB8, 0xC, 1, 1, 0},
          demute_asd[] = {0x3F, 5, 0x31, 0xB8, 0xC, 1, 0, 0};
  radon_asd_buf = make_msg_buf(0x6F1, 8, radon_asd);
  mute_asd_buf = make_msg_buf(0x6F1, 8, mute_asd);
  demute_asd_buf = make_msg_buf(0x6F1, 8, demute_asd);

  nivi_button_pressed_buf = make_msg_buf(0x28A, 2, generic_button_pressed);
  nivi_button_released_buf = make_msg_buf(0x28A, 2, generic_button_released);

  uint8_t power_down_cmd_a[] = {0x40, 3, 0x22, 0x3F, 0, 0, 0, 0},
          power_down_cmd_b[] = {0x40, 0x30, 0, 0, 0, 0, 0, 0},
          power_down_cmd_c[] = {0xEF, 3, 0x31, 5, 0, 0, 0, 0};
  power_down_cmd_a_buf = make_msg_buf(0x6F1, 8, power_down_cmd_a);
  power_down_cmd_b_buf = make_msg_buf(0x6F1, 8, power_down_cmd_b);
  power_down_cmd_c_buf = make_msg_buf(0x6F1, 8, power_down_cmd_c);

  uint8_t f_hu_nbt_reboot[] = {0x63, 2, 0x11, 1, 0, 0, 0, 0},
          jbe_reboot[] = {0, 2, 0x11, 1, 0, 0, 0, 0},
          ihka_5v_on[] = {0x78, 4, 0x30, 8, 7, 1},
          ihka_5v_off[] = {0x78, 4, 0x30, 8, 7, 0};
  f_hu_nbt_reboot_buf = make_msg_buf(0x6F1, 8, f_hu_nbt_reboot);
  jbe_reboot_buf = make_msg_buf(0x6F1, 8, jbe_reboot);
  ihka_5v_on_buf = make_msg_buf(0x6F1, 6, ihka_5v_on);
  ihka_5v_off_buf = make_msg_buf(0x6F1, 6, ihka_5v_off);

  uint8_t jbe_headlight_washer[] = {0, 5, 0x30, 2, 7, 0x28, 1, 0};
  jbe_headlight_washer_buf = make_msg_buf(0x6F1, 8, jbe_headlight_washer);
}


CAN_message_t make_msg_buf(uint16_t txID, uint8_t txLen, uint8_t* txBuf) {
  CAN_message_t tx_msg;
  tx_msg.id = txID;
  tx_msg.len = txLen;
  for (uint8_t i = 0; i < txLen; i++) {
      tx_msg.buf[i] = txBuf[i];
  }
  return tx_msg;
}


void initialize_can_handlers(void) {
  // If only one function (without arguments) is called to handle the message, it is called directly.
  // Otherwise, a wrapper (process_*) is called instead.

  kcan_handlers[0xAA] = process_kcan_AA;                                                                                            // 0xAA: (rpm/throttle pos) and torq_dvch - torque request, driver. Cycle time 100ms (KCAN).
  #if F_NBTE
    kcan_handlers[0xA8] = process_kcan_A8;                                                                                          // 0xA8: Crankshaft torque. Cycle time 100ms (KCAN).
  #endif
  kcan_handlers[0xEA] = evaluate_drivers_door_lock_status;
  kcan_handlers[0x130] = evaluate_terminal_clutch_keyno_status;                                                                     // 0x130: Terminal state, clutch status and key number. Cycle time 100ms.
  #if HDC
    kcan_handlers[0x193] = evaluate_cruise_control_status;                                                                          // 0x193: State of cruise control from KOMBI. Sent when changed.
    kcan_handlers[0x31A] = evaluate_hdc_button;                                                                                     // 0x31A: HDC button status from IHKA. Sent when changed.
  #endif
  #if FAKE_MSA
    kcan_handlers[0x195] = evaluate_msa_button;                                                                                     // 0x195: MSA button press from IHKA. Sent when changed.
  #endif
  kcan_handlers[0x19E] = evaluate_dsc_status;                                                                                       // 0x19E: DSC status (KCAN only). Cycle time 200ms.
  #if F_NBTE
    kcan_handlers[0x1A6] = send_f_distance_counters;                                                                                // 0x1A6: Distance message from DSC. Cycle time 100ms.
  #else
    kcan_handlers[0x1AA] = send_dme_power_ckm;                                                                                      // Time POWER CKM message with iDrive ErgoCommander (0x1AA). Sent at boot and Terminal R cycling.
  #endif
  kcan_handlers[0x1B4] = evaluate_kombi_status_message;                                                                             // 0x1B4: KOMBI status (indicated speed, handbrake). Cycle time 100ms (terminal R ON).
  #if F_NBTE_CCC_ZBE
    kcan_handlers[0x1B8] = convert_zbe1_message;                                                                                    // Convert old CCC controller data (0x1B8) for NBTE.
  #endif
  #if REVERSE_BEEP || DOOR_VOLUME
    kcan_handlers[0x1C6] = evaluate_pdc_warning;                                                                                    // 0x1C6: PDC acoustic warning.
  #endif
  kcan_handlers[0x1D0] = evaluate_engine_data;                                                                                      // 0x1D0: Engine temperatures and ambient pressure.
  kcan_handlers[0x1D6] = evaluate_mfl_button_press;                                                                                 // 0x1D6: MFL (Multi Function Steering Wheel) buttons. Cycle time 1s (idle), 100ms (pressed).
  #if MIRROR_UNDIM || F_NBTE
    kcan_handlers[0x1EE] = evaluate_indicator_stalk;                                                                                // 0x1EE: Indicator stalk status from FRM (KCAN only). Read-only, can't be changed.
  #endif
  kcan_handlers[0x1F6] = evaluate_indicator_status_dim;                                                                             // 0x1F6: Indicator status. Cycle time 1s. Sent when changed.
  kcan_handlers[0x202] = send_f_interior_ambient_light_brightness;                                                                  // 0x202: Interior ambient light brightness status sent by KOMBI.
  kcan_handlers[0x205] = evaluate_cc_gong_status;
  #if FRONT_FOG_LED_INDICATOR || FRONT_FOG_CORNER || DIM_DRL || HEADLIGHT_WASHING
    kcan_handlers[0x21A] = process_kcan_21A;                                                                                        // 0x21A: Light status sent by the FRM. Cycle time 5s (idle). Sent when changed.
  #endif
  #if AUTO_SEAT_HEATING_PASS
    kcan_handlers[0x22A] = evaluate_seat_heating_status;                                                                            // 0x22A: Passenger's seat heating status. Cycle time 10s (idle), 150ms (change).
  #endif
  #if AUTO_SEAT_HEATING
    kcan_handlers[0x232] = evaluate_seat_heating_status;                                                                            // 0x232: Driver's seat heating status. Cycle time 10s (idle), 150ms (change).
  #endif
  kcan_handlers[0x23A] = process_kcan_23A;                                                                                          // 0x23A: Remote fob function status. Sent 3x when changed.
  #if F_NBTE
    kcan_handlers[0x23D] = send_climate_popup_acknowledge;
    kcan_handlers[0x242] = evaluate_ihka_recirculation;
    kcan_handlers[0x2E6] = evaluate_ihka_auto_state;                                                                                // 0x2E6: Air distribution status message.
  #endif
  #if MIRROR_UNDIM
    kcan_handlers[0x286] = evaluate_electrochromic_dimming;                                                                         // 0x286: Dim signal from FZD. Cycle time 500ms (idle). Sent when changed.
  #endif
  #if WIPE_AFTER_WASH || INTERMITTENT_WIPERS || HEADLIGHT_WASHING
    kcan_handlers[0x2A6] = evaluate_wiper_stalk_status;                                                                             // 0x2A6: Wiper stalk status from SZL. Cycle time 1s (idle).
  #endif
  #if F_NBTE
    kcan_handlers[0x2C0] = send_f_lcd_brightness;                                                                                   // 0x2C0: Kombi LCD brightness. Cycle time 10s.
  #endif
  kcan_handlers[0x2CA] = evaluate_ambient_temperature;                                                                              // 0x2CA: Ambient temperature. Cycle time 1s.
  kcan_handlers[0x2F7] = process_kcan_2F7;                                                                                          // 0x2F7: Units from KOMBI. Sent 3x on Terminal R. Sent when changed.
  kcan_handlers[0x2F8] = evaluate_date_time;
  #if AUTO_SEAT_HEATING_PASS
    kcan_handlers[0x2FA] = evaluate_passenger_seat_status;                                                                          // 0x2FA: Passenger's seat occupancy and belt status. Cycle time 5s.
  #endif
  #if DOOR_VOLUME || AUTO_MIRROR_FOLD || IMMOBILIZER_SEQ || HOOD_OPEN_GONG
    kcan_handlers[0x2FC] = evaluate_door_status;                                                                                    // 0x2FC: Door, hood status sent by CAS. Cycle time 5s. Sent when changed.
  #endif
  #if F_VSW01
    kcan_handlers[0x2FD] = evaluate_vsw_status;                                                                                     // 0x2FD: VSW actual position status.
  #endif
  #if F_NBTE || F_NIVI || MIRROR_UNDIM || FRONT_FOG_CORNER
    kcan_handlers[0x314] = evaluate_rls_light_status;                                                                               // 0x314: RLS light status. Cycle time 3s. Sent when changed.
  #endif
  #if FTM_INDICATOR || F_NBTE
    kcan_handlers[0x31D] = evaluate_indicate_ftm_status;                                                                            // FTM status broadcast by DSC. Cycle time 5s (idle). Sent when changed.
  #endif
  kcan_handlers[0x326] = evaluate_edc_mode;                                                                                         // 0x326: EDC damper mode. Cycle tyme 200ms (idle).
  kcan_handlers[0x32E] = evaluate_interior_temperature;
  #if CONTROL_SHIFTLIGHTS
    kcan_handlers[0x332] = evaluate_update_shiftlight_sync;                                                                         // 0x332: Variable redline broadcast from DME. Cycle time 1s.
  #endif
  #if F_NBTE
    kcan_handlers[0x336] = process_bn2000_cc_display_list;
    kcan_handlers[0x338] = process_bn2000_cc_dialog;
  #endif
  #if !F_NBTE
    kcan_handlers[0x34A] = process_kcan_34A;
  #endif
  #if PDC_AUTO_OFF
    kcan_handlers[0x34F] = evaluate_handbrake_status;                                                                               // 0x34F: Handbrake handle status sent by JBE. Sent when changed.
  #endif
  #if F_NBTE
    kcan_handlers[0x35C] = evaluate_speed_warning_status;                                                                           // Fix: EVO does not support settings less than 15kph.
    kcan_handlers[0x388] = modify_vehicle_type;                                                                                     // Convert vehicle type information fed to KCAN2.
  #endif
  #if F_NBTE_CPS_VIN || F_KCAN2_VIN
    kcan_handlers[0x380] = modify_vehicle_vin;
  #endif
  #if AUTO_TOW_VIEW_RVC
    kcan_handlers[0x36D] = evaluate_pdc_distance;                                                                                   // 0x36D: Distance status sent by PDC. Sent when active.
    #if !F_NBTE
      kcan_handlers[0x38F] = store_rvc_settings_idrive;                                                                             // 0x38F: Camera settings request from iDrive. Sent when activating camera and when changed.
    #endif
    kcan_handlers[0x39B] = store_rvc_settings_trsvc;                                                                                // 0x39B: Camera settings/acknowledge from TRSVC. Sent when activating ignition and when changed.
  #endif
  #if !F_NBTE
    kcan_handlers[0x3A8] = update_dme_power_ckm;                                                                                    // 0x3A8: POWER M Key (CKM) setting from iDrive. Sent when changed.
  #endif
  #if PDC_AUTO_OFF || AUTO_TOW_VIEW_RVC || F_NBTE
    kcan_handlers[0x3AF] = process_kcan_3AF;                                                                                        // 0x3AF: PDC bus status. Cycle time 2s (idle). Sent when changed.
  #endif
  kcan_handlers[0x3B0] = process_kcan_3B0;                                                                                          // 0x3B0: Reverse gear status. Cycle time 1s (idle).
  #if F_NBTE || PTC_HEATER
    kcan_handlers[0x3B3] = evaluate_consumer_control;
  #endif
  kcan_handlers[0x3B4] = evaluate_battery_voltage;                                                                                  // 0x3B4: Battery voltage from DME. Cycle time 5s (idle), sent when ignition ON/OFF.
  kcan_handlers[0x3B6] = evaluate_front_windows_position;                                                                           // 0x3B6: Driver's power window status.
  kcan_handlers[0x3B8] = evaluate_front_windows_position;                                                                           // 0x3B8: Front passenger's power window status.
  kcan_handlers[0x3BA] = evaluate_sunroof_position;
  kcan_handlers[0x3BD] = evaluate_frm_consumer_shutdown;                                                                            // 0x3BD: Consumer shutdown message from FRM. Cycle time 5s (idle). Sent when changed.
  kcan_handlers[0x3BE] = evaluate_terminal_followup;                                                                                // 0x3BE: Terminal follow-up time from CAS. Cycle time 10s depending on bus activity.
  #if !F_NBTE
    kcan_handlers[0x3CA] = update_mdrive_message_settings_cic;                                                                      // 0x3CA: MDrive settings from iDrive (BN2000). Sent when changed.
  #endif
  kcan_handlers[0x3D7] = evaluate_door_lock_ckm;                                                                                    // 0x3D7: CKM setting status for door locks. Sent when changed.
  #if COMFORT_EXIT
    kcan_handlers[0x3DB] = evaluate_dr_seat_ckm;                                                                                    // 0x3DB: CKM setting for driver's seat. Sent when changed.
  #endif
  #if DIM_DRL || F_NBTE
    kcan_handlers[0x3DD] = evaluate_lights_ckm;                                                                                     // 0x3DD: CKM setting for lights. Sent when changed.
  #endif
  #if F_NBTE
    kcan_handlers[0x3DF] = evaluate_ihka_auto_ckm;                                                                                  // 0x3DF: CKM setting for AUTO blower speed.
  #endif
  kcan_handlers[0x3F1] = evaluate_hba_ckm;                                                                                          // 0x3F1: CKM setting for High beam assistant.
  #if TRSVC70
    kcan_handlers[0x586] = evaluate_trsvc_cc;
  #endif
  #if F_NIVI || F_NBTE
    kcan_handlers[0x650] = evaluate_vehicle_pitch_roll_angles;                                                                      // 0x650: SINE diagnostic responses. SINE is at address 0x50.
  #endif
  kcan_handlers[0x592] = process_dme_cc;
  kcan_handlers[0x640] = evaluate_power_down_response;                                                                              // 0x640: CAS diagnostic responses. Sent when requested.
  #if F_VSW01
    kcan_handlers[0x648] = process_kcan_648;                                                                                        // 0x648: VSW diagnostic responses. Sent when requested.
  #endif
  #if DOOR_VOLUME && !F_NBTE
    kcan_handlers[0x663] = evaluate_audio_volume_cic;                                                                               // 0x663: iDrive diagnostic response. Sent when requested.
  #endif
  #if AUTO_MIRROR_FOLD || FRONT_FOG_CORNER
    kcan_handlers[0x672] = process_kcan_672;                                                                                        // 0x672: FRM diagnostic responses. Sent when requested.
  #endif

  #if F_NBTE
    memset(kcan_to_kcan2_forward_filter_list, 1, sizeof(kcan_to_kcan2_forward_filter_list));                                        // Unconditionally forward all messages except the ones below.

    kcan_to_kcan2_forward_filter_list[0xA9] = 0;                                                                                    // BN2000 only, TORQUE_2.
    kcan_to_kcan2_forward_filter_list[0xAA] = 0;                                                                                    // BN2000 only, engine status and torques.
    kcan_to_kcan2_forward_filter_list[0xA8] = 0;                                                                                    // BN2000 only, TORQUE_1 KCAN.
    kcan_to_kcan2_forward_filter_list[0xC4] = 0;                                                                                    // BN2000 only, steering angle.
    kcan_to_kcan2_forward_filter_list[0xC8] = 0;                                                                                    // BN2000 only, SZL angle.
    kcan_to_kcan2_forward_filter_list[0xCE] = 0;                                                                                    // BN2000 only, wheel speeds.
    kcan_to_kcan2_forward_filter_list[0x130] = 0;                                                                                   // BN2000 terminal status. Rarely used by some PL6 modules (old ZBE and other KCAN modules?).
    kcan_to_kcan2_forward_filter_list[0x195] = 0;                                                                                   // BN2000 only, MSA button.
    kcan_to_kcan2_forward_filter_list[0x19E] = 0;                                                                                   // BN2000 Status DSC / BN2010 control subnetworks.
    kcan_to_kcan2_forward_filter_list[0x1A0] = 0;                                                                                   // BN2000 Speed / BN2010 Gearbox check-control.
    kcan_to_kcan2_forward_filter_list[0x1B4] = 0;                                                                                   // BN2000 only, KOMBI status.
    kcan_to_kcan2_forward_filter_list[0x1B6] = 0;                                                                                   // BN2000 Engine electrical current flow. Unknown in BN2010. It causes NBTE to block phone calls.
    kcan_to_kcan2_forward_filter_list[0x1D0] = 0;                                                                                   // BN2000 only, engine data.
    kcan_to_kcan2_forward_filter_list[0x1D6] = 0;                                                                                   // MFL buttons for next/previous are swappend for NBTE.
    kcan_to_kcan2_forward_filter_list[0x2B2] = 0;                                                                                   // BN2000 only, wheel brake pressures.
    kcan_to_kcan2_forward_filter_list[0x2C0] = 0;                                                                                   // BN2000 only, LCD brightness.
    kcan_to_kcan2_forward_filter_list[0x2F3] = 0;                                                                                   // BN2000 gear shift instruction / BN2010 gyro.
    kcan_to_kcan2_forward_filter_list[0x2F4] = 0;                                                                                   // BN2000 A/C control / BN2010 ZGW high speed sync.
    kcan_to_kcan2_forward_filter_list[0x2F7] = 0;                                                                                   // KOMBI units. Requires further processing.
    kcan_to_kcan2_forward_filter_list[0x317] = 0;                                                                                   // BN2000 only, PDC button.
    kcan_to_kcan2_forward_filter_list[0x31D] = 0;                                                                                   // BN2000 only, FTM status.
    kcan_to_kcan2_forward_filter_list[0x326] = 0;                                                                                   // BN2000 only, EDC status.
    kcan_to_kcan2_forward_filter_list[0x332] = 0;                                                                                   // BN2000 only, variable redline.
    kcan_to_kcan2_forward_filter_list[0x336] = 0;                                                                                   // CC list display. Requires further processing.
    kcan_to_kcan2_forward_filter_list[0x338] = 0;                                                                                   // CC dialog display. Requires further processing.
    kcan_to_kcan2_forward_filter_list[0x35C] = 0;                                                                                   // Speed warning setting. Requires further processing.
    kcan_to_kcan2_forward_filter_list[0x380] = 0;                                                                                   // VIN number. May require further processing.
    kcan_to_kcan2_forward_filter_list[0x388] = 0;                                                                                   // Vehicle Type. Requires further processing.
    kcan_to_kcan2_forward_filter_list[0x399] = 0;                                                                                   // BN2000 MDrive / BN2010 Status energy voltage.
    kcan_to_kcan2_forward_filter_list[0x3B3] = 0;                                                                                   // DME consumer control. Requires further processing.
    kcan_to_kcan2_forward_filter_list[0x3CC] = 0;
    kcan_to_kcan2_forward_filter_list[0x3DD] = 0;                                                                                   // Lights CKM. Requires further processing.
    kcan_to_kcan2_forward_filter_list[0x3F7] = 0;
    for (int k = 0x3FF; k < 0x6F1; k++) {                                                                                           // BN2000 CCs, diagnosis responses and NM from various modules.
      kcan_to_kcan2_forward_filter_list[k] = 0;
    }
    kcan_to_kcan2_forward_filter_list[0x5E0] = 1;                                                                                   // Kombi response to 0x5E3?
  #endif

  #if FRONT_FOG_CORNER || F_NIVI || F_NBTE
    ptcan_handlers[0xC4] = process_ptcan_C4;
  #endif
  ptcan_handlers[0x1A0] = process_ptcan_1A0;
  #if HDC
    ptcan_handlers[0x194] = evaluate_cruise_stalk_message;
  #endif
  #if SERVOTRONIC_SVT70
    ptcan_handlers[0x58E] = send_svt_kcan_cc_notification;
    ptcan_handlers[0x60E] = process_ptcan_60E;                                                                                      // Forward Diagnostic responses from SVT module (0x60E) to DCAN.
  #endif
  #if CUSTOM_MONITORING_CC
    ptcan_handlers[0x612] = evaluate_dme_boost_response;
  #endif
  
  #if F_NBTE
    #if F_NBTE_CCC_ZBE
      kcan2_handlers[0x273] = send_zbe_acknowledge;                                                                                 // HU tried to initialize a controller.
    #endif
    kcan2_handlers[0x291] = evaluate_idrive_units;
    kcan2_handlers[0x2B8] = evaluate_speed_warning_setting;
    kcan2_handlers[0x31A] = evaluate_f_pdc_function_request;
    kcan2_handlers[0x34A] = process_kcan2_34A;                                                                                      // 0x34A: HU-OMAP: GPS position, sent regardless of Terminal status. Cycle time 1s.
    #if AUTO_TOW_VIEW_RVC
      kcan2_handlers[0x38F] = store_rvc_settings_idrive;
    #endif
    kcan2_handlers[0x39E] = evaluate_idrive_zero_time;                                                                              // 0x39E: New date/time from HU.
    kcan2_handlers[0x3DC] = evaluate_idrive_lights_settings;
    kcan2_handlers[0x42F] = update_mdrive_message_settings_nbt;                                                                     // 0x42F: MDrive settings from HU (BN2010).
    kcan2_handlers[0x5E3] = process_hu_kombi_settings;                                                                              // 0x5E3: Used by the HU to set KOMBI settings such as Independent Ventilation. Similar to 5E2.
    kcan2_handlers[0x635] = process_kcan2_635;                                                                                      // 0x635: TBX diagnostic response.
    kcan2_handlers[0x663] = process_kcan2_663;                                                                                      // 0x663: HU diagnostic response.
    #if F_NIVI
      kcan2_handlers[0x657] = process_kcan2_657;                                                                                    // 0x657: NVE diagnostic response.
    #endif

    memset(kcan2_to_kcan_forward_filter_list,
           1, sizeof(kcan2_to_kcan_forward_filter_list));
    kcan2_to_kcan_forward_filter_list[0xBF] = 0;                                                                                    // BN2010 touchpad data message.
    kcan2_to_kcan_forward_filter_list[0x2B8] = 0;                                                                                   // Speed warning setting. Modified some bytes to fit BN2000 format.
    kcan2_to_kcan_forward_filter_list[0x317] = 0;                                                                                   // This message is used for the BN2010 ZBE instead of PDC activation. Causes sporadic PDC activation.
    kcan2_to_kcan_forward_filter_list[0x31A] = 0;                                                                                   // PDC bus status request. Converted to 0x3AE.
    kcan2_to_kcan_forward_filter_list[0x39E] = 0;                                                                                   // Time/date, checked for zero before forwarding.
    kcan2_to_kcan_forward_filter_list[0x3DC] = 0;                                                                                   // Light CKM setting. Converted to use 0x5E2. Stored last byte to correct 0x3DD.
    kcan2_to_kcan_forward_filter_list[0x42F] = 0;                                                                                   // BN2010 MDrive settings.
    for (int i = 0x500; i < 0x600; i++) {                                                                                           // BN2010 NM messages.
      kcan2_to_kcan_forward_filter_list[i] = 0;
    }
    kcan2_to_kcan_forward_filter_list[0x657] = 0;
    for (int i = 0x6F1; i < 0x800; i++) {                                                                                           // Irrelevant data.
      kcan2_to_kcan_forward_filter_list[i] = 0;
    }
  #endif
}


void kcan_write_msg(const CAN_message_t &msg) {
  if (msg.id == 0x6F1 && !diag_transmit) {
    if (msg.buf[0] == 0x41 && (msg.buf[2] == 0x30 || msg.buf[2] == 0x31)) {                                                         // Exception for alarm jobs.
    } else if (msg.buf[0] == 0x48) {                                                                                                // Exception for VSW01 diagnosis.
    } else {
      serial_log("6F1 message not sent to KCAN due to OBD tool presence.", 1);
      can_debug_print_buffer(msg);
      return;
    }
  }
  uint8_t result = KCAN.write(msg);
  if (result != 1) {
    if (kcan_retry_counter < 10) {                                                                                                  // Safeguard to avoid polluting the network in case of unrecoverable issue.
      m = {msg, millis() + 100};
      kcan_resend_txq.push(&m);
      kcan_retry_counter++;
      sprintf(serial_debug_string, "KCAN write failed for ID: %lX with error %d. Re-sending.", msg.id, result);
      serial_log(serial_debug_string, 1);
      can_debug_print_buffer(msg);
    } else {
      serial_log("KCAN resend max counter exceeded.", 1);
    }
    kcan_error_counter++;
  } else {
    #if defined(USB_TRIPLE_SERIAL)
      if (millis() >= 2000 && SerialUSB2.dtr()) {
        if (slcan_bus == 1) {
          xfer_can2tty(msg);                                                                                                        // This allows the SLCAN interface to see messages sent by the program on KCAN.
        }
      }
    #endif

    kcan_error_counter = 0;
    kcan_resend_txq.flush();
  }
}


void kcan2_write_msg(const CAN_message_t &msg) {

  // if ( msg.id == 0x12F || msg.id == 0x130 || msg.id == 0x560) {} else { return; }                                                  // Testing with minimal busload

  if (kl30g_cutoff_imminent) {                                                                                                      // KL30G is about to be cut. Stop sending KCAN2 messages so modules power OFF gracefully.
    return;                                                                                                                         // Since 3B3 was sent with ST_ENERG_PWMG=2, modules are probably OFF and transmission would fail.
  }

  #if F_NBTE
    if (kcan2_mode == MCP_NORMAL) {
      byte send_buf[msg.len];

      for (uint8_t i = 0; i < msg.len; i++) {
        send_buf[i] = msg.buf[i];
      }

      if (CAN_OK != KCAN2.sendMsgBuf(msg.id, 0, msg.len, send_buf)) {
        if (vehicle_awakened_timer >= 500) {                                                                                        // There are some errors writing just after boot. Ignore.
          sprintf(serial_debug_string, "KCAN2 write failed for ID: %lX.", msg.id);
          serial_log(serial_debug_string, 1);
          can_debug_print_buffer(msg);
          kcan2_error_counter++;
        }
      }

      #if defined(USB_TRIPLE_SERIAL)
      if (millis() >= 2000 && SerialUSB2.dtr()) {
        if (slcan_bus == 2) {
          xfer_can2tty(msg);                                                                                                        // This allows the SLCAN interface to see messages sent by the program on KCAN2.
        }
      }
      #endif
    }
  #endif
}


void ptcan_write_msg(const CAN_message_t &msg) {
  if (ptcan_mode == 1) {
    if (msg.id == 0x6F1 && !diag_transmit) {
      if (msg.buf[0] == 0x17 && msg.buf[2] == 0x30 && (msg.buf[1] == 4 || msg.buf[1] == 2)) {                                       // Exception for EKP disable.
      } else if (msg.buf[0] == 0xE || msg.buf[0] == 0x57) {                                                                         // Exception for SVT70 and NiVi diag.
      } else {
        serial_log("6F1 message not sent to PTCAN due to OBD tool presence.", 2);
        can_debug_print_buffer(msg);
        return;
      }
    }

    uint8_t result = PTCAN.write(msg);
    if (result != 1) {
      if (ptcan_retry_counter < 10) {                                                                                               // Safeguard to avoid polluting the network in case of unrecoverable issue.
        m = {msg, millis() + 50};
        ptcan_resend_txq.push(&m);
        ptcan_retry_counter++;
        sprintf(serial_debug_string, "PTCAN write failed for ID: %lX with error %d. Re-sending.", msg.id, result);
        serial_log(serial_debug_string, 1);
        can_debug_print_buffer(msg);
      } else {
        serial_log("PTCAN resend max counter exceeded.", 1);
      }
      ptcan_error_counter++;
    } else {
      ptcan_retry_counter = 0;
      ptcan_resend_txq.flush();
    }
  } else {
    sprintf(serial_debug_string, "PTCAN transmission for ID: %lX aborted while transceiver is in standby.", msg.id);
    serial_log(serial_debug_string, 1);
    can_debug_print_buffer(msg);
  }
}


void dcan_write_msg(const CAN_message_t &msg) {
  if (dcan_mode == 1) {
    if (vehicle_awake) {
      uint8_t result = DCAN.write(msg);
      if (result != 1) {
        if (dcan_retry_counter < 10) {                                                                                              // Safeguard to avoid polluting the network in case of unrecoverable issue.
          m = {msg, millis() + 50};
          dcan_resend_txq.push(&m);
          dcan_retry_counter++;
          sprintf(serial_debug_string, "DCAN write failed for ID: %lX with error %d.", msg.id, result);
          serial_log(serial_debug_string, 1);
          can_debug_print_buffer(msg);
        } else {
          serial_log("DCAN resend max counter exceeded.", 1);
        }
        dcan_error_counter++;
      } else {
        dcan_retry_counter = 0;
        dcan_resend_txq.flush();
      }
    } else {
      sprintf(serial_debug_string, "DCAN write failed for ID: %lX because vehicle is asleep.", msg.id);
      serial_log(serial_debug_string, 1);
    }
  } else {
    sprintf(serial_debug_string, "DCAN transmission for ID: %lX aborted while transceiver is in standby.", msg.id);
    serial_log(serial_debug_string, 1);
    can_debug_print_buffer(msg);
  }
}


void check_can_resend_queues(void) {
  if (!kcan_resend_txq.isEmpty()) {
    kcan_resend_txq.peek(&delayed_tx);
    if (millis() >= delayed_tx.transmit_time) {
      kcan_write_msg(delayed_tx.tx_msg);
      kcan_resend_txq.drop();
    }
  }
  if (!ptcan_resend_txq.isEmpty()) {
    ptcan_resend_txq.peek(&delayed_tx);
    if (millis() >= delayed_tx.transmit_time) {
      kcan_write_msg(delayed_tx.tx_msg);
      ptcan_resend_txq.drop();
    }
  }
  if (!dcan_resend_txq.isEmpty()) {
    dcan_resend_txq.peek(&delayed_tx);
    if (millis() >= delayed_tx.transmit_time) {
      kcan_write_msg(delayed_tx.tx_msg);
      dcan_resend_txq.drop();
    }
  }
}


void can_debug_print_buffer(const CAN_message_t &msg) {
  if (LOGLEVEL >= 3) {
    Serial.print(" Buffer: ");
    for ( uint8_t i = 0; i < msg.len; i++ ) {
      Serial.print(msg.buf[i], HEX); Serial.print(" ");
    }
    Serial.println();
  }
}


void convert_f_nbt_network_management(void) {
  uint8_t hu_nm[] = {0x62, 0x42, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  if (!hu_bn2000_nm_initialized) {
    hu_nm[1] = 1;
  } else {
    hu_nm[0] = hu_bn2000_nm_next_neighbour;
    if ((!terminal_r && !hu_ent_mode) || kl30g_cutoff_imminent) {
      if ((hu_bn2000_bus_sleep_ready_timer >= HU_ENT_MODE_TIMEOUT) || kl30g_cutoff_imminent) {                                      // Give the driver time to reactivate the HU otherwise the car would kill KCAN immediately.
        hu_nm[1] = 0x52;                                                                                                            // This timeout is also triggered when the FRM wakes the KCAN before deep sleep!
        if (!hu_bn2000_bus_sleep_active) {
          hu_bn2000_bus_sleep_active = true;
          serial_log("HU ENT_MODE timed out, allowing KCAN sleep.", 2);
        }
      } else {
        hu_bn2000_bus_sleep_active = false;
      }
    } else {
      hu_bn2000_bus_sleep_active = false;
    }
  }

  CAN_message_t hu_nm_buf = make_msg_buf(0x4E2, 8, hu_nm);
  kcan_write_msg(hu_nm_buf);

  #if defined(USB_TRIPLE_SERIAL)
    if (millis() >= 2000 && SerialUSB2.dtr()) {
      if (slcan_bus == 2) {
        xfer_can2tty(hu_nm_buf);                                                                                                    // For diagnostics, transmit this message to SLCAN.
      }
    }
  #endif
}


void send_f_kombi_network_management(void) {
  #if F_VSW01
    if (terminal_r) {                                                                                                               // If not Terminal R, allow the VSW to sleep. The last position it was in remains.
      kcan_write_msg(f_kombi_network_management_buf);
    }
  #endif
  #if F_NBTE || F_NIVI
    kcan2_write_msg(f_kombi_network_management_buf);
  #endif
}
