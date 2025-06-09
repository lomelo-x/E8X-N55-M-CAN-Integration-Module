// Engine/driveline functions go here.


void evaluate_engine_status(void) {
  RPM = ((uint16_t)k_msg.buf[5] << 8) | (uint16_t)k_msg.buf[4];
  e_throttle_pedal_position = k_msg.buf[3] * 0.39063;
  uint8_t idle_status = (k_msg.buf[6] & 0xF) >> 2;                                                                                  // Get the first two bits of the second half of this byte.
  if (idle_status == 0) {
    engine_idling = true;
  } else {
    engine_idling = false;
  }
}


void toggle_mdrive_message_active(void) {
  if (mdrive_status) {                                                                                                              // Turn OFF MDrive.
    serial_log("Status MDrive OFF.", 2);
    mdrive_message_bn2000[1] -= 1;                                                                                                  // Decrement bytes 1 (6MT, DSC mode) and 4 (SVT) to deactivate.
    mdrive_message_bn2000[4] -= 0x10;
    mdrive_status = mdrive_power_active = false;
    #if FRM_AHL_MODE
      kcan_write_msg(frm_ckm_ahl_komfort_buf);
      serial_log("Set AHL back to comfort mode.", 2);
    #endif
    #if ASD89_MDRIVE
      if (diag_transmit) {
        kcan_write_msg(mute_asd_buf);
        serial_log("Muted ASD output.", 2);
      }
    #endif
    if (mdrive_power[cas_key_number] == 0x30) {
      exhaust_flap_sport = false;
    } else if (mdrive_power[cas_key_number] == 0x10) {
      console_power_mode = restore_console_power_mode;
    }                                                                                                                               // Else, POWER unchanged
  } else {                                                                                                                          // Turn ON MDrive.
    serial_log("Status MDrive ON.", 2);
    if (mdrive_power[cas_key_number] == 0x20) {                                                                                     // POWER in Sport.
      mdrive_power_active = true;
    } else if (mdrive_power[cas_key_number] == 0x30) {                                                                              // POWER Sport+.
      #if ASD89_MDRIVE
        if (diag_transmit) {
          kcan_write_msg(demute_asd_buf);
          serial_log("De-muted ASD output.", 2);
        }
      #endif
      exhaust_flap_sport = true;                                                                                                    // Exhaust flap always open in Sport+
      mdrive_power_active = true;
    } else if (mdrive_power[cas_key_number] == 0x10) {
      restore_console_power_mode = console_power_mode;                                                                              // We'll need to return to its original state when MDrive is turned OFF.
      console_power_mode = false;                                                                                                   // Turn OFF POWER from console too.
    }                                                                                                                               // Else, POWER unchanged.

    #if FRM_AHL_MODE
    if (mdrive_svt[cas_key_number] >= 0xF1) {                                                                                       // Headlights will move faster if Servotronic is set to Sport.
      kcan_write_msg(frm_ckm_ahl_sport_buf);
      serial_log("Set AHL to sport mode.", 2);
    }
    #endif

    mdrive_message_bn2000[1] += 1;
    mdrive_message_bn2000[4] += 0x10;
    mdrive_status = true;
  }

  veh_mode_timer = 500;
}


void toggle_mdrive_dsc_mode(void) {
  if (mdrive_status) {
    if (mdrive_dsc[cas_key_number] == 7) {                                                                                          // DSC OFF requested.
      if (!send_dsc_mode(1)) {
        serial_log("MDrive: Failed to change DSC mode during stabilisation.", 1);
        #if F_NBTE
          send_cc_message("DSC mode change refused while stabilising.", true, 3000);
        #endif
      }
      #if CONTROL_SHIFTLIGHTS
      else {
        if (!shiftlights_segments_active && dsc_program_status != 1) {                                                              // Don't interfere with existing shiftlights and don't flash if changing other settings.
          activate_shiftlight_segments(shiftlights_max_flash_buf);
          ignore_shiftlights_off_counter = 2;
        }
      }
      #endif
    } else if (mdrive_dsc[cas_key_number] == 0x13) {                                                                                // DSC MDM (DTC in non-M) requested.
      if (!send_dsc_mode(4)) {
        serial_log("MDrive: Failed to change DSC mode during stabilisation.", 1);
        #if F_NBTE
          send_cc_message("DSC mode change refused while stabilising.", true, 3000);
        #endif
      }
    } else if (mdrive_dsc[cas_key_number] == 0xB) {                                                                                 // DSC ON requested.
      send_dsc_mode(0);
    }
  } else {
    if (mdrive_dsc[cas_key_number] == 0x13 || mdrive_dsc[cas_key_number] == 7) {                                                    // If MDrive was set to change DSC, restore back to DSC ON.
      if (dsc_program_status != 0) {
        send_dsc_mode(0);
      }
    }
  }
}


void evaluate_mfl_button_press(void) {
  #if F_NBTE
    if (k_msg.buf[1] == 0xC) {    
      if (k_msg.buf[0] == 0xE0) {                                                                                                   // Up Button.
        k_msg.buf[0] = 0xD0;
      } else if (k_msg.buf[0] == 0xD0) {                                                                                            // Down Button.
        k_msg.buf[0] = 0xE0;
      }
    }
    kcan2_write_msg(k_msg);                                                                                                         // Processing complete, forward message to HU.
  #endif

  if (k_msg.buf[1] == 0x4C) {                                                                                                       // Source / M button is pressed.

    if (!ignore_m_hold) {
      received_m_press = true;                                                                                                      // Ignore further pressed messages until the button is released.
    }

    if (m_mfl_held_count > 10 && !ignore_m_hold) {                                                                                  // Each count is about 100ms
      if (ignition
          #if IMMOBILIZER_SEQ
          && immobilizer_released
          #endif
         ) {
        show_mdrive_settings_screen();
        received_m_press = false;                                                                                                   // This show the settings screen without changing the state of MDrive.
      }
    } else {
      m_mfl_held_count++;
    }
  } else if (k_msg.buf[1] == 0xC && k_msg.buf[0] == 0xC0) {                                                                         // Button is released.
    if (received_m_press) {
      if (ignition
          #if IMMOBILIZER_SEQ
          && immobilizer_released                                                                                                   // Disable normal M button function when used for immobilizer.
          #endif
          ) {
        toggle_mdrive_message_active();
        toggle_mdrive_dsc_mode();
        mdrive_message_bn2000_timer = mdrive_message_bn2010_timer = 10000;
      }

      #if IMMOBILIZER_SEQ
        uint8_t activate_release_counter = IMMOBILIZER_SEQ_NUMBER - 1;
        if (!immobilizer_released) {
          activate_release_counter = alarm_active ? IMMOBILIZER_SEQ_ALARM_NUMBER - 1 : IMMOBILIZER_SEQ_NUMBER - 1;

          if (immobilizer_pressed_release_count < activate_release_counter) {
            immobilizer_pressed_release_count++;
          } else {
            release_immobilizer();
          }
        } else {                                                                                                                    // Allow re-activation before sleep mode.
          if (immobilizer_activate_release_timer >= 3000) {
            if (!terminal_r && immobilizer_persist) {
              if (immobilizer_pressed_activate_count < activate_release_counter) {
                immobilizer_pressed_activate_count++;
              } else {
                activate_immobilizer();
                play_cc_gong(1);
              }
            }
          }
        }
      #endif
    }
    m_mfl_held_count = 0;
    received_m_press = ignore_m_hold = false;
  }
}


void update_mdrive_settings_can_message(void) {
  mdrive_message_bn2000[1] = mdrive_dsc[cas_key_number] - 2;                                                                        // Difference between iDrive settting and BN2000 CAN message is always 2
                                                                                                                                    // DSC: 1 unchanged, 5 OFF, 0x11 MDM, 9 On
  mdrive_message_bn2000[2] = mdrive_power[cas_key_number];                                                                          // Copy POWER as is.
  mdrive_message_bn2000[3] = mdrive_edc[cas_key_number];                                                                            // Copy EDC as is.
  if (mdrive_svt[cas_key_number] == 0xE9) {
    mdrive_message_bn2000[4] = 0x41;                                                                                                // SVT normal, MDrive OFF.
  } else if (mdrive_svt[cas_key_number] == 0xF1 || mdrive_svt[cas_key_number] == 0xF2) {
    mdrive_message_bn2000[4] = 0x81;                                                                                                // SVT sport (or Sport+ from NBTE), MDrive OFF.
  }

  #if F_NBTE
    // DSC
    if (mdrive_dsc[cas_key_number] == 7) {                                                                                          // DSC OFF.
      mdrive_message_bn2010[1] = 1;
    } else if (mdrive_dsc[cas_key_number] == 0x13) {                                                                                // DSC MDM (DTC in non-M).
      mdrive_message_bn2010[1] = 3;
    } else if (mdrive_dsc[cas_key_number] == 0xB) {                                                                                 // DSC ON.
      mdrive_message_bn2010[1] = 2;
    }

    // Engine and Drivelogic
    if (mdrive_power[cas_key_number] == 0 || mdrive_power[cas_key_number] == 0x10) {
      mdrive_message_bn2010[2] = 1 << 4;                                                                                            // Efficient
    } else if (mdrive_power[cas_key_number] == 0x20) {
      mdrive_message_bn2010[2] = 2 << 4;                                                                                            // Sport                                                                                           
    } else if (mdrive_power[cas_key_number] == 0x30) {
      mdrive_message_bn2010[2] = 3 << 4;                                                                                            // Sport+
    }
    mdrive_message_bn2010[2] |= 1;                                                                                                  // Drivelogic level 1

    // EPAS / SVT
    if (mdrive_svt[cas_key_number] == 0xE9) {
      mdrive_message_bn2010[3] = 1 << 4;                                                                                            // Comfort
    } else if (mdrive_svt[cas_key_number] == 0xF1) {
      mdrive_message_bn2010[3] = 2 << 4;                                                                                            // Sport
    } else if (mdrive_svt[cas_key_number] == 0xF2) {
      mdrive_message_bn2010[3] = 3 << 4;                                                                                            // Sport+
    }

    // EDC
    if (mdrive_edc[cas_key_number] == 0x20 || mdrive_edc[cas_key_number] == 0x21) {
      mdrive_message_bn2010[3] |= 1;                                                                                                // Comfort                         
    } else if (mdrive_edc[cas_key_number] == 0x22) {
      mdrive_message_bn2010[3] |= 2;                                                                                                // Sport
    } else if (mdrive_edc[cas_key_number] == 0x2A) {
      mdrive_message_bn2010[3] |= 3;                                                                                                // Sport+
    }
  #endif

  if (mdrive_status) {
    mdrive_message_bn2000[1] += 1;
    mdrive_message_bn2000[4] += 0x10;
  }
}


void show_mdrive_settings_screen(void) {
  if (diag_transmit) {
    #if IMMOBILIZER_SEQ
    if (immobilizer_released) {
    #endif
      serial_log("Steering wheel M button held. Showing settings screen.", 2);
      #if F_NBTE
        kcan2_write_msg(idrive_mdrive_settings_menu_nbt_a_buf);
        mdrive_settings_requested = true;
      #else
        kcan_write_msg(idrive_mdrive_settings_menu_cic_a_buf);                                                                      // Send steuern_menu job to iDrive.
        kcan_write_msg(idrive_mdrive_settings_menu_cic_b_buf);
        if (!hu_application_died && diag_transmit) {
          unsigned long time_now = millis();
          m = {idrive_button_sound_buf, time_now + 500};
          idrive_txq.push(&m);
        }
      #endif
    #if IMMOBILIZER_SEQ
    }
    #endif
    ignore_m_hold = true;
  }
}


void show_mdrive_settings_screen_evo(void) {
  if (mdrive_settings_requested) {
    if (k_msg.buf[0] == 0xF1 && k_msg.buf[1] == 0x30) {
      kcan2_write_msg(idrive_mdrive_settings_menu_nbt_b_buf);
      kcan2_write_msg(idrive_mdrive_settings_menu_nbt_c_buf);
      mdrive_settings_requested = false;
      if (!hu_application_died) {
        unsigned long time_now = millis();
        m = {idrive_horn_sound_buf, time_now + 500};
        idrive_txq.push(&m);
      }
    } else {
      mdrive_settings_requested = false;
    }
  }
}


void send_mdrive_status_message(void) {
  if (mdrive_message_bn2000_timer >= 10000) {                                                                                       // Original cycle time is 10s (idle) for BN2000.
    if (ptcan_mode == 1) {
      mdrive_message_bn2000[0] += 10;
      if (mdrive_message_bn2000[0] > 0xEF) {                                                                                        // Alive(first half of byte) must be between 0..E.
        mdrive_message_bn2000[0] = 0;
      }
      can_checksum_update(0x399, 6, mdrive_message_bn2000);                                                                         // Recalculate checksum. No module seems to check this - MDSC?
      ptcan_write_msg(make_msg_buf(0x399, 6, mdrive_message_bn2000));                                                               // Send to PTCAN like the DME would. EDC will receive. KOMBI will receive on KCAN through JBE.
      mdrive_message_bn2000_timer = 0;
    }
  }
  #if F_NBTE
    if (mdrive_message_bn2010_timer >= 1000) {                                                                                      // If this message times out (e.g during sketch upload) settings are reset to default.
      if (kcan2_mode == MCP_NORMAL) {
        mdrive_message_bn2010[0] = f_mdrive_alive_counter;
        kcan2_write_msg(make_msg_buf(0x42E, 8, mdrive_message_bn2010));
        f_mdrive_alive_counter == 0xE ? f_mdrive_alive_counter = 0 
                                      : f_mdrive_alive_counter++;
        mdrive_message_bn2010[7] = 0xE8;                                                                                            // Reset the acknowledge beep.
        mdrive_message_bn2010_timer = 0;
      }
    }
  #endif
}


void execute_mdrive_settings_changed_actions() {
  if (mdrive_status) {                                                                                                              // Perform actions that are impacted by changing settings while MDrive is active.
    if (mdrive_power[cas_key_number] == 0x20) {                                                                                     // POWER in Sport.
      mdrive_power_active = true;
    } else if (mdrive_power[cas_key_number] == 0x30) {                                                                              // POWER Sport+.
      #if ASD89_MDRIVE
        if (diag_transmit) {
          kcan_write_msg(demute_asd_buf);
          serial_log("De-muted ASD output.", 2);
        }
      #endif
      exhaust_flap_sport = true;                                                                                                    // Exhaust flap always open in Sport+
      mdrive_power_active = true;
    } else if (mdrive_power[cas_key_number] == 0x10) {
      restore_console_power_mode = console_power_mode;                                                                              // We'll need to return to its original state when MDrive is turned OFF.
      console_power_mode = false;                                                                                                   // Turn OFF POWER from console too.
      mdrive_power_active = false;
      #if ASD89_MDRIVE
        if (diag_transmit) {
          kcan_write_msg(mute_asd_buf);
          serial_log("Muted ASD output.", 2);
        }
      #endif
      exhaust_flap_sport = false;
    }
    toggle_mdrive_dsc_mode();                                                                                                       // Change DSC to the new state.

    if (mdrive_svt[cas_key_number] == 0xE9) {
      #if FRM_AHL_MODE
        kcan_write_msg(frm_ckm_ahl_komfort_buf);
        serial_log("Set AHL to comfort mode with SVT setting of Normal.", 2);
      #endif
    } else if (mdrive_svt[cas_key_number] >= 0xF1) {
      #if FRM_AHL_MODE
        kcan_write_msg(frm_ckm_ahl_sport_buf);
        serial_log("Set AHL to sport mode with SVT setting of Sport/Sport+.", 2);
      #endif
    }
  }
}


void update_mdrive_message_settings_nbt(void) {
  if (ignition) {
    if (k_msg.buf[0] >> 4 == 0) {                                                                                                   // Mdrive 1 settings sent by HU.
      if ((k_msg.buf[4] >> 4) == 0xE) {                                                                                             // Reset requested.
        reset_mdrive_settings();
        mdrive_message_bn2010[7] = 0xE9;                                                                                            // Sound to acknowledge reset.
      } else if ((k_msg.buf[4] >> 4) == 0xC){
        if (k_msg.buf[0] != 0) {                                                                                                    // DSC settings were updated.
          if (k_msg.buf[0] == 1) {                                                                                                  // DSC OFF.
            mdrive_dsc[cas_key_number] = 7;
          } else if (k_msg.buf[0] == 2) {                                                                                           // DSC ON.
            mdrive_dsc[cas_key_number] = 0xB;
          } else if (k_msg.buf[0] == 3) {                                                                                           // DSC MDM.
            mdrive_dsc[cas_key_number] = 0x13;
          }
        }

        else if(k_msg.buf[1] != 0) {                                                                                                // Engine settings were updated.
          mdrive_power[cas_key_number] = (k_msg.buf[1] >> 4) * 0x10;
        }

        else if ((k_msg.buf[2] & 0xF) != 0) {                                                                                       // Chassis settings were updated.
          if ((k_msg.buf[2] & 0xF) == 1) {                                                                                          // Comfort.
            mdrive_edc[cas_key_number] = 0x21;
          } else if ((k_msg.buf[2] & 0xF) == 2) {                                                                                   // Sport.
            mdrive_edc[cas_key_number] = 0x22;
          } else if ((k_msg.buf[2] & 0xF) == 3) {                                                                                   // Sport+.
            mdrive_edc[cas_key_number] = 0x2A;
          }
        }

        else if (((k_msg.buf[2] >> 4) & 0x0F) != 0) {                                                                               // Steering settings were updated.
          if (((k_msg.buf[2] >> 4) & 0x0F) == 1) {                                                                                  // Comfort.
            mdrive_svt[cas_key_number] = 0xE9;
          } else if ((k_msg.buf[2] >> 4) == 2) {                                                                                    // Sport.
            mdrive_svt[cas_key_number] = 0xF1;
          } else if ((k_msg.buf[2] >> 4) == 3) {                                                                                    // Sport+.
            mdrive_svt[cas_key_number] = 0xF2;
          }
        }

        sprintf(serial_debug_string, "Received iDrive settings: DSC 0x%X POWER 0x%X EDC 0x%X SVT 0x%X.", 
            mdrive_dsc[cas_key_number], mdrive_power[cas_key_number], mdrive_edc[cas_key_number], mdrive_svt[cas_key_number]);
        serial_log(serial_debug_string, 3);

        update_mdrive_settings_can_message();
        execute_mdrive_settings_changed_actions();
      }
    }
    else if (k_msg.buf[0] >> 4 == 0xF) {}                                                                                           // Current settings requested from DME. Send message.
    else {
      return;                                                                                                                       // Ignore settings for MDrive 2.
    }
  }
  mdrive_message_bn2000_timer = mdrive_message_bn2010_timer = 10000;
}


void reset_mdrive_settings(void) {
  #if F_NBTE                                                                                                                        // NBTE does not have "Unchanged" settings.
    mdrive_dsc[cas_key_number] = 0xB;                                                                                               // DSC ON
    mdrive_power[cas_key_number] = 0x10;                                                                                            // Normal
    mdrive_edc[cas_key_number] = 0x21;                                                                                              // Comfort
    mdrive_svt[cas_key_number] = 0xE9;                                                                                              // Normal
  #else
    mdrive_dsc[cas_key_number] = 3;                                                                                                 // Unchanged
    mdrive_power[cas_key_number] = 0;                                                                                               // Unchanged
    mdrive_edc[cas_key_number] = 0x20;                                                                                              // Unchanged
    mdrive_svt[cas_key_number] = 0xE9;                                                                                              // Normal
    dme_ckm[cas_key_number][0] = 0xF1;                                                                                              // Normal
  #endif
  serial_log("Reset MDrive settings to defaults.", 3);
  update_mdrive_settings_can_message();
  execute_mdrive_settings_changed_actions();
}


// Written by amg6975
// https://www.spoolstreet.com/threads/MDrive-and-mdm-in-non-m-cars.7155/post-107037
void can_checksum_update(uint16_t canid, uint8_t len,  uint8_t *message) {
  message[0] &= 0xF0;                                                                                                               // Remove checksum from byte.
  // Add up all bytes and the CAN ID
  uint16_t checksum = canid;
  for (uint8_t i = 0; i < len; i++) {
    checksum += message[i];
  }                                 
  checksum = (checksum & 0x00FF) + (checksum >> 8); //add upper and lower Bytes
  checksum &= 0x00FF; //throw away anything in upper Byte
  checksum = (checksum & 0x000F) + (checksum >> 4); //add first and second nibble
  checksum &= 0x000F; //throw away anything in upper nibble

  message[0] += checksum;                                                                                                           // Add the checksum back to Byte0.
}


void send_power_mode(void) {
  if (ptcan_mode == 1) {
    power_mode_only_dme_veh_mode[0] += 0x10;                                                                                        // Increase alive counter.
    if (power_mode_only_dme_veh_mode[0] > 0xEF) {                                                                                   // Alive(first half of byte) must be between 0..E.
      power_mode_only_dme_veh_mode[0] = 0;
    }

    if (console_power_mode || mdrive_power_active) {                                                                                // Activate sport throttle mapping if POWER from console ON or Sport/Sport+ selected in MDrive (active).
      power_mode_only_dme_veh_mode[1] = 0xF2;                                                                                       // Sport
      digitalWrite(POWER_LED_PIN, HIGH);
    } else {
      power_mode_only_dme_veh_mode[1] = 0xF1;                                                                                       // Normal
      digitalWrite(POWER_LED_PIN, LOW);
    }

    can_checksum_update(DME_FAKE_VEH_MODE_CANID, 2, power_mode_only_dme_veh_mode);
    ptcan_write_msg(make_msg_buf(DME_FAKE_VEH_MODE_CANID, 2, power_mode_only_dme_veh_mode));
  }
}


void check_immobilizer_status(void) {
  if (vehicle_awakened_timer >= 2000) {                                                                                             // Delay ensures that time passed after Teensy (re)started to receive messages.
    if (!immobilizer_released) {
      if (vehicle_awakened_timer >= 8000) {                                                                                         // Delay so we don't interfere with normal DWA behavior indicating alarm faults. Also when doing 30G reset.

        // Check frm_consumer_shutdown so that we don't activate when half-waking. Open trunk, lock button while locked...
        if (!frm_consumer_shutdown && !alarm_led_disable_on_lock) {
          if (alarm_led_message_timer >= 98000) {                                                                                   // Visual indicator that the immobilizer is active. Timeout 120s. Send periodically to keep it ON.
            kcan_write_msg(alarm_led_on_buf);
            alarm_led_message_timer = 0;
          }
        }
      }

      // This could be temporarily bypassed if the car is started very very quickly and driven?
      // It will reactivate once indicated_speed <= immobilizer_max_speed. Think Speed (1994)...
      if (!vehicle_moving || (indicated_speed <= immobilizer_max_speed)) {                                                          // Make sure we don't cut this OFF while the car is in (quick) motion!!!
        if (immobilizer_timer >= immobilizer_send_interval) {
          if (terminal_r) {
            if (engine_running == 2 && engine_run_timer >= 2000) {                                                                  // This ensures LPFP can still prime when unlocking, opening door, Terminal R, Ignition ON etc.
              ptcan_write_msg(ekp_pwm_off_buf);
              serial_log("EKP is disabled.", 2);
            }

            // Visual indicators with Terminal R, 15.
            kcan_write_msg(key_cc_on_buf);                                                                                          // Keep sending this message so that CC is ON until disabled.
          }
          immobilizer_timer = 0;
          immobilizer_send_interval = 2000;
        }
      }
    }
  }
  if (!ekp_txq.isEmpty()) {
    ekp_txq.peek(&delayed_tx);
    if (millis() >= delayed_tx.transmit_time) {
      ptcan_write_msg(delayed_tx.tx_msg);
      ekp_txq.drop();
    }
  }
  if (!alarm_led_txq.isEmpty()) {
    alarm_led_txq.peek(&delayed_tx);
    if (millis() >= delayed_tx.transmit_time) {
      kcan_write_msg(delayed_tx.tx_msg);
      alarm_led_txq.drop();
    }
  }
  if (!alarm_warnings_txq.isEmpty()) {
    alarm_warnings_txq.peek(&delayed_tx);
    if (millis() >= delayed_tx.transmit_time) {
      kcan_write_msg(delayed_tx.tx_msg);
      alarm_warnings_txq.drop();
    }
  }
  if (!alarm_siren_txq.isEmpty()) {
    alarm_siren_txq.peek(&delayed_tx);
    if (millis() >= delayed_tx.transmit_time) {
      kcan_write_msg(delayed_tx.tx_msg);
      alarm_siren_txq.drop();
    }
  }
}


void reset_key_cc(void) {
  kcan_write_msg(key_cc_off_buf);
}


void activate_immobilizer(void) {
  immobilizer_released = false;
  immobilizer_pressed_release_count = 0;
  alarm_led_txq.flush();
  ekp_txq.flush();
  EEPROM.update(30, immobilizer_released);
  update_eeprom_checksum();                                                                                                         // Update now in case we lose power before sleep.
  immobilizer_activate_release_timer = 0;
  serial_log("Immobilizer activated.", 2);
  alarm_led_message_timer = 100000;
}


void release_immobilizer(void) {
  immobilizer_released = true;
  immobilizer_pressed_activate_count = 0;
  EEPROM.update(30, immobilizer_released);                                                                                          // Save to EEPROM directly in case program crashes.
  update_eeprom_checksum();
  ptcan_write_msg(ekp_return_to_normal_buf);
  unsigned long time_now = millis();
  ekp_txq.flush();
  m = {ekp_return_to_normal_buf, time_now + 100};                                                                                   // Make sure these messages are received.
  ekp_txq.push(&m);
  m = {ekp_return_to_normal_buf, time_now + 300};
  ekp_txq.push(&m);
  serial_log("Immobilizer released. EKP control restored to DME.", 2);
  kcan_write_msg(alarm_siren_return_control_buf);                                                                                   // The return control message can be sent without interfering with operation.
  alarm_siren_txq.flush();                                                                                                          // Clear pending siren requests.
  m = {alarm_siren_return_control_buf, time_now + 100};
  alarm_siren_txq.push(&m);
  m = {alarm_siren_return_control_buf, time_now + 300};
  alarm_siren_txq.push(&m);
  if (alarm_active) {
    serial_log("Alarm OFF.", 2);
  }
  alarm_after_engine_stall = alarm_active = false;
  kcan_write_msg(alarm_led_return_control_buf);
  alarm_led_txq.flush();
  m = {alarm_led_return_control_buf, time_now + 500};                                                                               // Delay since we already sent a 6F1 to DWA.
  alarm_led_txq.push(&m);
  m = {alarm_led_return_control_buf, time_now + 800};
  alarm_led_txq.push(&m);
  kcan_write_msg(key_cc_off_buf);
  if (terminal_r || hu_ent_mode) {
    #if F_NBTE
      send_cc_message("Immobilizer released. Engine start permitted.", true, 3500);                                                 // Timed roughly so it matches the start ready CC dismiss.
    #endif
    m = {start_cc_on_buf, time_now + 500};
    alarm_warnings_txq.push(&m);
    m = {start_cc_off_buf, time_now + 2000};
    alarm_warnings_txq.push(&m);
    serial_log("Sent start ready CC.", 2);
  } else {
    play_cc_gong(1);
  }
  immobilizer_activate_release_timer = 0;
}


void enable_alarm_after_stall(void) {
  if (!immobilizer_released) {
    alarm_after_engine_stall = true;
    serial_log("Engine started with immobilizer still active. Alarm will sound after engine stalls.", 2);    
    kcan_write_msg(key_cc_on_buf);
    kcan_write_msg(alarm_led_on_buf);
    unsigned long time_now = millis();
    m = {cc_triple_gong_buf, time_now + 1500};                                                                                      // Audible alert of impending stall.
    alarm_warnings_txq.push(&m);
  }
}


void execute_alarm_after_stall(void) {
  if (alarm_after_engine_stall) {
    alarm_led_txq.flush();                                                                                                          // Clear pending gongs.
    unsigned long time_now = millis();
    m = {alarm_siren_on_buf, time_now};
    alarm_siren_txq.push(&m);
    m = {alarm_siren_on_buf, time_now + 500};                                                                                       // Make sure the alarm request is received.
    alarm_siren_txq.push(&m);
    for (uint8_t i = 1; i < 10; i++) {                                                                                              // Alarm test job times out after 30s. Make it blast for 5 min.
      m = {alarm_siren_on_buf, time_now + 31000 * i};                                                                               // Siren job must not be active for another to be ran.
      alarm_siren_txq.push(&m);
    }
    serial_log("Alarm siren and hazards ON.", 2);
    #if F_NBTE
      send_cc_message("Immobilizer alarm tripped!", true, 10000);
    #endif
    alarm_active = true;
  }
}


void send_f_powertrain_2_status(void) {
  if (f_data_powertrain_2_timer >= 1000) {
    uint8_t f_data_powertrain_2[] = {0, 0, 0x80, 0, 0, 0, 0, 0x8C};                                                                 // 6MC2DL0B pg. 1384. Byte7 max rpm: 50 * 140(0x8C) = 7000.

    f_data_powertrain_2[1] = 1 << 4 | f_data_powertrain_2_alive_counter;                                                            // Combine ST_ECU_DT_PT_2 (0001 - State of normal operation) and ALIV_DT_PT_2.
    f_data_powertrain_2_alive_counter == 0xE ? f_data_powertrain_2_alive_counter = 0 
                                             : f_data_powertrain_2_alive_counter++;

    if (engine_running == 2) {
      f_data_powertrain_2[2] |= 2;                                                                                                  // Engine running.
      if (!engine_idling) {
        f_data_powertrain_2[3] = 1 << 6;                                                                                            // Not idling.
      }
      f_data_powertrain_2[3] |= 2;
    } else if (engine_running == 1) {
      f_data_powertrain_2[3] = 1 << 6;                                                                                              // Not idling.
      f_data_powertrain_2[3] |= 1;                                                                                                  // Engine starting.
    } else {
      f_data_powertrain_2[3] = 1 << 6;                                                                                              // Not idling.
    }

    if (clutch_pressed) {
      f_data_powertrain_2[3] |= 1 << 2;
    }

    f_data_powertrain_2[3] |= 3 << 4;                                                                                               // On manual cars, this signal (ST_ILK_STRT_DRV) is set to invalid.

    f_data_powertrain_2[4] = engine_coolant_temperature;                                                                            // Engine coolant temperature (C): value - 48.
    f_data_powertrain_2[5] = engine_oil_temperature;                                                                                // Engine oil temperature (C): value - 48.

    f_data_powertrain_2[6] = 0x2F;                                                                                                  // Engine start allowed. Other signals unused on 6MT (0xF).

    f_data_powertrain_2_crc.restart();
    for (uint8_t i = 1; i < 8; i++) {
      f_data_powertrain_2_crc.add(f_data_powertrain_2[i]);
    }
    f_data_powertrain_2[0] = f_data_powertrain_2_crc.calc();

    CAN_message_t f_data_powertrain_2_buf = make_msg_buf(0x3F9, 8, f_data_powertrain_2);
    #if F_NBTE || F_NIVI
      kcan2_write_msg(f_data_powertrain_2_buf);
    #endif
    f_data_powertrain_2_timer = 0;
  }
}


void send_f_torque_1(void) {                                                                                                        // 6MC2DL0B pg. 1393.
  if (f_torque_1_timer >= 100) {                                                                                                    // Original BN2010 cycle time is 20ms for KCAN2 and PTCAN.
    uint8_t f_torque_1[] = {0, 0, 0, 0, 0, 0, 0, 0xF2};                                                                             // Byte7 QU_AVL_RPM_ENG_CRSH hardcoded to 2 - Signal valid.
    if (vehicle_awakened_timer <= 5000) {                                                                                           // Force the status to initialization after the car just woke.
      f_torque_1[7] = 0xF8;
    }

    f_torque_1[1] = 0xF << 4 | f_torque_1_alive_counter;                                                                            // Combine FREI (0xF - unused) and ALIV_TORQ_CRSH_1.
    f_torque_1_alive_counter == 0xE ? f_torque_1_alive_counter = 0 
                                  : f_torque_1_alive_counter++;
   
    int16_t raw_torque = 0x800 + (int)round(engine_torque_nm) * 2;
 
    // Engine torque
    f_torque_1[2] = raw_torque & 0xFF;                                                                                              // LE encoded.
    f_torque_1[3] = (raw_torque & 0xFF) << 4 | (raw_torque >> 8);
    f_torque_1[4] = (raw_torque >> 8);

    // Engine RPM (raw, not scaled by 0.25)
    f_torque_1[5] = RPM & 0xFF;                                                                                                     // Transmit in LE.
    f_torque_1[6] = RPM >> 8;

    f_torque_1_crc.restart();
    for (uint8_t i = 1; i < 8; i++) {
      f_torque_1_crc.add(f_torque_1[i]);
    }
    f_torque_1[0] = f_torque_1_crc.calc();

    CAN_message_t f_torque_1_buf = make_msg_buf(0xA5, 8, f_torque_1);
    kcan2_write_msg(f_torque_1_buf);
    f_torque_1_timer = 0;
  }
}


void send_f_throttle_pedal(void) {                                                                                                  // 6MC2DL0B pg. 1379.
  if (f_throttle_pedal_timer >= 50) {                                                                                               // Original BN2010 cycle time is 20ms for KCAN2 and PTCAN.
    uint8_t f_throttle_pedal[] = {0, 0, 0, 0x10, 0, 0xF0, 0x7F, 0xC0};                                                              // Byte3 QU_AVL_ANG_ACPD = 1
    if (vehicle_awakened_timer <= 5000) {                                                                                           // Force the status to initialization after the car just woke.
      f_throttle_pedal[3] = 0x80;
    }

    f_throttle_pedal[1] = 1 << 4 | f_throttle_pedal_alive_counter;                                                                  // Combine 1 (Normal operation) and ALIV_ANG_ACPD.
    f_throttle_pedal_alive_counter == 0xE ? f_throttle_pedal_alive_counter = 0 
                                  : f_throttle_pedal_alive_counter++;
    

    uint16_t scaled_value = (e_throttle_pedal_position / 99.22) * 100;                                                              // Max value reported by DME is 99.22%.
    scaled_value = constrain((int)round(scaled_value / 0.025), 0, 0xFFE);                                                           // 12-bit boundary check. 0xFFF - signal invalid.

    f_throttle_pedal[2] = scaled_value & 0xFF;                                                                                      // AVL_ANG_ACPD LSB
    f_throttle_pedal[3] = f_throttle_pedal[3] | (scaled_value >> 8);                                                                // QU_AVL_ANG_ACPD | AVL_ANG_ACPD MSB

    // AVL_ANG_ACPD_VIRT probably represents the DME internally scaled throttle. Assuming linear here...
    f_throttle_pedal[4] = scaled_value & 0xFF;                                                                                      // AVL_ANG_ACPD_VIRT LSB
    f_throttle_pedal[5] = f_throttle_pedal[5] | scaled_value >> 8;                                                                  // AVL_ANG_ACPD_VIRT MSB

    if (scaled_value > 0) {
      f_throttle_pedal[7] = 0xC1;                                                                                                   // Accelerator_pedal_(driver)_request,_no_FAS_request
    }

    f_throttle_pedal_crc.restart();
    for (uint8_t i = 1; i < 8; i++) {
      f_throttle_pedal_crc.add(f_throttle_pedal[i]);
    }
    f_throttle_pedal[0] = f_throttle_pedal_crc.calc();

    CAN_message_t f_throttle_pedal_buf = make_msg_buf(0xD9, 8, f_throttle_pedal);
    kcan2_write_msg(f_throttle_pedal_buf);
    f_throttle_pedal_timer = 0;
  }
}


void send_f_driving_dynamics_switch_evo(void) {
  if (f_driving_dynamics_timer >= 1000) {
    uint8_t f_driving_dynamics[] = {0xFF, 0xFF, 8, 0, 0xF3, 0xFF, 0xFF};

    if (edc_mode == 9 || edc_mode == 0xB) {
      f_driving_dynamics[2] = 0x10;                                                                                                 // VDC Sport
    }

    if (mdrive_status) {
      if (console_power_mode || mdrive_power_active) {
        if (mdrive_power[cas_key_number] == 0x20) {
          f_driving_dynamics[2] |= 9;                                                                                               // Drivetrain Sport
        } else if (mdrive_power[cas_key_number] == 0x30) {
          f_driving_dynamics[2] |= 0xB;                                                                                             // Drivetrain Sport+
        }
      }
    } else if (console_power_mode) {
      f_driving_dynamics[2] |= 9;
    }

    if (dsc_program_status == 1) {
      f_driving_dynamics[3] = 0x47;                                                                                                 // DSC OFF, ICM OFF, ICM mode sport.
    }

    if (f_driving_dynamics_ignore < 3) {                                                                                            // Briefly show the DSC OFF popup but preserve FDS for other modules.
      f_driving_dynamics[4] = 0xF6;                                                                                                 // DSC OFF
      f_driving_dynamics_ignore++;
    } else {
      if (mdrive_status) {
        if (console_power_mode || mdrive_power_active) {
          if (mdrive_power[cas_key_number] == 0x20) {
            f_driving_dynamics[4] = 4;                                                                                              // FDS Sport
          } else if (mdrive_power[cas_key_number] == 0x30) {
            f_driving_dynamics[4] = 5;                                                                                              // FDS Sport+
          }
        }
      } else if (console_power_mode) {
        f_driving_dynamics[4] = 4;
      }
    }

    // This (TRACTION) does not match MDM. F8X does not have a popup for MDM. Only 35up/CLAR cars do.
    //   f_driving_dynamics[3] = 0x10;                                                                                                // DTC, ICM ON, ICM basis.
    //   f_driving_dynamics[4] = 0xF1;                                                                                                // TRACTION

    CAN_message_t f_driving_dynamics_buf = make_msg_buf(0x3A7, 7, f_driving_dynamics);
    kcan2_write_msg(f_driving_dynamics_buf);
    f_driving_dynamics_timer = 0;
  }
}


void control_exhaust_flap_user(void) {
  if (engine_running == 2) {
    if (exhaust_flap_sport) {                                                                                                       // Flap always open in sport mode.
      if (exhaust_flap_action_timer >= 500) {
        if (!exhaust_flap_open) {
          actuate_exhaust_solenoid(LOW);
          serial_log("Opened exhaust flap with MDrive.", 2);
        }
      }
    }
  } else {
    #if QUIET_START
      if (terminal_r && !exhaust_flap_sport) {                                                                                      // Close the flap when Terminal R activates, allow bypass with MDrive.
        if (exhaust_flap_open) {
          actuate_exhaust_solenoid(HIGH);                                                                                           // Close the flap (if vacuum still available).
          serial_log("Quiet start enabled. Exhaust flap closed.", 2);
        }
      } else {
        if (!exhaust_flap_open) {
          actuate_exhaust_solenoid(LOW);
          serial_log("Released exhaust flap from quiet start.", 2);
        }
      }
    #endif
  }
}


void control_exhaust_flap_rpm(void) {
  if (engine_running == 2) {
    if (!exhaust_flap_sport) {
      if (exhaust_flap_action_timer >= exhaust_flap_action_interval) {                                                              // Avoid vacuum drain, oscillation and apply startup delay.
        if (RPM >= EXHAUST_FLAP_QUIET_RPM) {                                                                                        // Open at defined rpm setpoint.
          if (!exhaust_flap_open) {
            actuate_exhaust_solenoid(LOW);
            serial_log("Exhaust flap opened at RPM setpoint.", 2);
          }
        } else {
          if (exhaust_flap_open) {
            actuate_exhaust_solenoid(HIGH);
            serial_log("Exhaust flap closed.", 2);
          }
        }
      }
    }
  }
}


void actuate_exhaust_solenoid(bool activate) {
  digitalWrite(EXHAUST_FLAP_SOLENOID_PIN, activate);
  exhaust_flap_action_timer = 0;
  exhaust_flap_open = !activate;                                                                                                    // Flap position is the inverse of solenoid state. When active, the flap is closed.
}


void evaluate_engine_data(void) {
  engine_coolant_temperature = k_msg.buf[0];
  engine_oil_temperature = k_msg.buf[1];
  ambient_pressure = (k_msg.buf[3] * 2) + 598;

  uint8_t engine_running_ = 0;
  bitWrite(engine_running_, 0, bitRead(k_msg.buf[2], 4));
  bitWrite(engine_running_, 1, bitRead(k_msg.buf[2], 5));

  if (engine_running_ == 3) { engine_running_ = 0; }                                                                                // Treat invalid status as Engine OFF.

  if (engine_running_ != engine_running) {
    if (engine_running_ == 2) {
      engine_run_timer = 0;
      #if CONTROL_SHIFTLIGHTS
        shiftlight_startup_animation();                                                                                             // Show off shift light segments during engine startup.
      #endif
      #if NEEDLE_SWEEP
        needle_sweep_animation();
      #endif
      exhaust_flap_action_timer = 0;                                                                                                // Start tracking the exhaust flap.
      serial_log("Engine started.", 2);
      #if IMMOBILIZER_SEQ
        enable_alarm_after_stall();
      #endif
      custom_info_cc_timer = 3000;
    } else if (engine_running_ == 1) {
      serial_log("Engine cranking.", 2);
    } else {                                                                                                                        // Engine stalled or was stopped. Stopped = 0 rpm.
      serial_log("Engine stopped.", 2);
      asd_rad_on_initialized = false;                                                                                               // By default the ASD module turns off RAD_ON when the engine stops.
      #if IMMOBILIZER_SEQ
        execute_alarm_after_stall();
      #endif
    }
    engine_running = engine_running_;
  }
}


void send_dme_boost_request(void) {
  unsigned long boost_request_interval = 1000;
  if (engine_running == 2) {
    boost_request_interval = 200;
  }
  if (boost_request_timer >= boost_request_interval) {
    if (diag_transmit) {
      ptcan_write_msg(dme_boost_request_a_buf);
      dme_boost_requested = true;
    }
    boost_request_timer = 0;
  }
}


void evaluate_dme_boost_response(void) {
  if (dme_boost_requested) {
    if (pt_msg.buf[0] == 0xF1 && pt_msg.buf[1] == 0x10 && pt_msg.buf[4] == 0x19) {
      engine_cp_sensor = (int)round((pt_msg.buf[6] << 8 | pt_msg.buf[7]) * 0.0390625);
      ptcan_write_msg(dme_boost_request_b_buf);
    } else if (pt_msg.buf[0] == 0xF1 && pt_msg.buf[1] == 0x21) {
      engine_manifold_sensor = (int)round((pt_msg.buf[2] << 8 | pt_msg.buf[3]) * 0.0390625);
      intake_air_temperature = (int)round((pt_msg.buf[4] << 8 | pt_msg.buf[5]) * 0.1);
      dme_boost_requested = false;
    } else {
      dme_boost_requested = false;
    }
  }
}


void modify_vehicle_type(void) {                                                                                                    // 0x388 6MC2DL0B pg. 1323.
  if (k_msg.buf[0] == 0xE) {
    k_msg.buf[0] = 0x22;                                                                                                            // F32
  } else if (k_msg.buf[0] == 8) {
    k_msg.buf[0] = 0x25;                                                                                                            // F30
  } else if (k_msg.buf[0] == 0xD) {
    k_msg.buf[0] = 0x21;                                                                                                            // F31
  } else if (k_msg.buf[0] == 0xF) {
    k_msg.buf[0] = 0x23;                                                                                                            // F33
  }

  k_msg.buf[3] = (7 << 4) | (k_msg.buf[3] & 0xF);                                                                                   // TL power class (F8X).

  if (k_msg.buf[6] == 0x23) {
    k_msg.buf[6] = 0x1E;                                                                                                            // Change 3500cc to 3000cc.
  }

  #if F_NBTE
    kcan2_write_msg(k_msg);
  #endif
}
