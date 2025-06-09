// Functions relating to the headunit go here.


void send_zbe_acknowledge(void) {                                                                                                   // Without this response, rotation data is ignored.
  uint8_t zbe_response[] = {0xE1, 0x9D, 0, 0xFF};
  zbe_response[2] = k_msg.buf[7];
  kcan2_write_msg(make_msg_buf(0x277, 4, zbe_response));
  kcan_write_msg(ccc_zbe_wake_buf);                                                                                                 // Reset the ZBE counter upon HU's request.
}


void send_volume_request_door(void) {
  if (terminal_r || hu_ent_mode) {
    if (diag_transmit) {
      if (!idrive_txq.isEmpty()) {                                                                                                  // If there are pending volume change actions we need to wait for them to complete.                                                                                           
        m = {vol_request_buf, millis() + 900};
        idrive_txq.push(&m);
      } else {
        if (volume_request_door_timer >= 300) {                                                                                     // Basic debounce to account for door not fully shut.
          #if F_NBTE
            kcan2_write_msg(vol_request_buf);
          #else
            kcan_write_msg(vol_request_buf);
          #endif
          volume_request_door_timer = 0;
          serial_log("Requested volume from iDrive with door status change.", 2);
        }
      }
    }
  }
}


void evaluate_audio_volume_nbt(void) {
  if (k_msg.buf[0] == 0xF1 && k_msg.buf[4] == 0xA0 && k_msg.buf[5] == 0x39) {                                                       // status_volumeaudio response.
    if (k_msg.buf[6] > 0) {                                                                                                         // Otherwise, probably muted.
      uint8_t volume_change[] = {0x63, 6, 0x31, 1, 0xA0, 0x36, 0, 0};
      if (!volume_reduced) {
        if (peristent_volume != k_msg.buf[6]) {
          peristent_volume = k_msg.buf[6];
          sprintf(serial_debug_string, "Received new audio volume: 0x%X.", k_msg.buf[6]);
          serial_log(serial_debug_string, 3);
        }
        if (pdc_tone_on || gong_active) {                                                                                           // If PDC beeps are active, volume change has no effect.
          return;
        }
        if (k_msg.buf[6] >= 5) {                                                                                                    // Don't reduce if already very low.
          if (left_door_open || right_door_open) {
            volume_change[6] = floor(k_msg.buf[6] * 0.75);                                                                          // Softer decrease.
            if (volume_change[6] > 0x33) {
              volume_change[6] = 0x33;
            }
            unsigned long time_now = millis();
            m = {make_msg_buf(0x6F1, 8, volume_change), time_now + 200};
            idrive_txq.push(&m);
            volume_restore_offset = (k_msg.buf[6] % 2) == 0 ? 0 : 1;                                                                // Volumes adjusted from faceplate go up by 1 while MFL goes up by 2.
            volume_change[6] = floor(k_msg.buf[6] / 2);                                                                             // Reduce volume to 50%.
            if ((volume_change[6] + volume_restore_offset) > 0x33) {                                                                // Don't blow the speakers out if something went wrong...
              volume_change[6] = 0x33;
              volume_restore_offset = 0;
            }
            m = {make_msg_buf(0x6F1, 8, volume_change), time_now + 600};
            idrive_txq.push(&m);
            volume_reduced = true;
            volume_changed_to = volume_change[6];                                                                                   // Save this value to compare when door is closed back.
            sprintf(serial_debug_string, "Reducing audio volume with door open to: 0x%X.", volume_changed_to);
            serial_log(serial_debug_string, 3);
          }
        }
      } else {
        peristent_volume = k_msg.buf[6] * 2 + volume_restore_offset;
        if (!left_door_open && !right_door_open) {
          if (k_msg.buf[6] == volume_changed_to) {
            volume_change[6] = floor(k_msg.buf[6] * 1.5);                                                                           // Softer increase.
            if (volume_change[6] > 0x33) {
              volume_change[6] = 0x33;
            }
            unsigned long time_now = millis();
            m = {make_msg_buf(0x6F1, 8, volume_change), time_now + 200};
            idrive_txq.push(&m);
            volume_change[6] = k_msg.buf[6] * 2 + volume_restore_offset;
            if (volume_change[6] > 0x33) {
              volume_change[6] = 0x33;                                                                                              // Set a nanny in case the code goes wrong. 0x33 is pretty loud...
            }
            m = {make_msg_buf(0x6F1, 8, volume_change), time_now + 600};                                                            // Make sure the restore is received.
            idrive_txq.push(&m);
            sprintf(serial_debug_string, "Restoring audio volume with door closed. to: 0x%X.", volume_change[6]);
            serial_log(serial_debug_string, 3);
          } else {
            peristent_volume = k_msg.buf[6];                                                                                        // User changed volume while door was opened.
            sprintf(serial_debug_string, "Volume changed by user while door was open to: 0x%X.", k_msg.buf[6]);
            serial_log(serial_debug_string, 3);
          }
          volume_reduced = false;
        }
      }
    }
  }
}


void check_idrive_queue(void) {
  if (!idrive_txq.isEmpty()) {
    if (diag_transmit) {
      idrive_txq.peek(&delayed_tx);
      if (millis() >= delayed_tx.transmit_time) {
        if (vehicle_awake) {
          #if F_NBTE
            kcan2_write_msg(delayed_tx.tx_msg);
            if (delayed_tx.tx_msg.buf[4] == 0xA0 && delayed_tx.tx_msg.buf[5] == 0x36) {
              serial_log("Sent volume change job to iDrive.", 2);
            }
          #else
            kcan_write_msg(delayed_tx.tx_msg);
            if (delayed_tx.tx_msg.buf[3] == 0x23) {
              serial_log("Sent volume change job to iDrive.", 2);
            }
          #endif
        }
        idrive_txq.drop();
      }
    } else {
      idrive_txq.flush();
    }
  }
}


void check_hu_application_alive_monitor(void) {
  #if F_NBTE
    if (kcan2_mode == MCP_NORMAL) {                                                                                                 // No reason to check the alive timer if KCAN2 is in standby.
  #endif
      if (hu_application_watchdog >= 3000) {                                                                                        // This message should be received every 1-2s.
        if (!hu_application_died) {
          hu_application_died = true;
          serial_log("iDrive application watchdog timed out.", 2);
          initial_volume_set = false;
          asd_initialized = false;
          asd_rad_on_initialized = false;
          #if F_VSW01 && F_VSW01_MANUAL
            vsw_switch_input(4);
          #endif
          idrive_run_timer = 0;                                                                                                     // Keep track of the iDrive's boot time.
        }
      } else {
        if (hu_application_died) {                                                                                                  // It's back.
          hu_application_died = false;
          serial_log("iDrive application running again.", 2);
          kcan2_write_msg(f_lights_ckm_delayed_msg);
          mdrive_message_bn2000_timer = mdrive_message_bn2010_timer = 10000;
        }
      }
  #if F_NBTE
    }
  #endif
}


void initialize_asd_mdrive(void) {
  if (!asd_initialized) {
    if (diag_transmit) {
      kcan_write_msg(mute_asd_buf);
      serial_log("Muted ASD output on init.", 2);
      asd_initialized = true;
    }
  }
}


void initialize_asd_rad_on(void) {
  if (!asd_rad_on_initialized) {
    if (diag_transmit && !frm_consumer_shutdown && (terminal_r || hu_ent_mode)) {                                                   // This should not run during the 30G reset wake operation.
      unsigned long time_now = millis();
      m = {radon_asd_buf, time_now};
      radon_txq.push(&m);
      m = {radon_asd_buf, time_now + 1000};                                                                                         // A slight delay is needed after engine OFF.
      radon_txq.push(&m);
      serial_log("Sending RAD_ON request to ASD module.", 2);
      asd_rad_on_initialized = true;
    }
  }
}


void check_radon_queue(void) {
  if (!radon_txq.isEmpty()) {
    radon_txq.peek(&delayed_tx);
    if (millis() >= delayed_tx.transmit_time) {
      kcan_write_msg(delayed_tx.tx_msg);
      radon_txq.drop();
    }
  }
}


void vsw_switch_input(uint8_t input) {
  uint8_t vsw_switch_position[] = {input, 0, 0, 0, 0, 0, 0, vsw_switch_counter};
  if (vsw_current_input != input) {
    kcan_write_msg(make_msg_buf(0x2FB, 8, vsw_switch_position));
    vsw_switch_counter == 0xFE ? vsw_switch_counter = 0xF1 : vsw_switch_counter++;
    sprintf(serial_debug_string, "Sent VSW/%d (%s) request.", input, vsw_positions[input]);
    serial_log(serial_debug_string, 3);
  }
}


void evaluate_vsw_status(void) {
  vsw_current_input = k_msg.buf[0];
}


void send_nbt_vin_request(void) {
  if (!donor_vin_initialized) {
    if (nbt_vin_request_timer >= 3000) {
      kcan2_write_msg(nbt_vin_request_buf);
      nbt_vin_request_timer = 0;
      requested_donor_vin = true;
    }
  }
}


void evaluate_nbt_vin_response(void) {
  if (requested_donor_vin) {
    if (k_msg.buf[0] == 0xF1 && k_msg.buf[1] == 0x10 && k_msg.buf[4] == 0xF1 && k_msg.buf[5] == 0x90) {                             // Received the first part of the VIN response. Send acknowledge
      uint8_t vin_continue_response[] = {0x63, 0x30, 0x08, 0x14, k_msg.buf[4], k_msg.buf[5], k_msg.buf[6], k_msg.buf[7]};
      kcan2_write_msg(make_msg_buf(0x6F1, 8, vin_continue_response));
      receiving_donor_vin = true;
    } else if (k_msg.buf[0] == 0xF1 && k_msg.buf[1] == 0x22 && receiving_donor_vin) {
      DONOR_VIN[0] = k_msg.buf[4];
      DONOR_VIN[1] = k_msg.buf[5];
      DONOR_VIN[2] = k_msg.buf[6];
      DONOR_VIN[3] = k_msg.buf[7];
    } else if (k_msg.buf[0] == 0xF1 && k_msg.buf[1] == 0x23 && receiving_donor_vin) {
      DONOR_VIN[4] = k_msg.buf[2];
      DONOR_VIN[5] = k_msg.buf[3];
      DONOR_VIN[6] = k_msg.buf[4];
      receiving_donor_vin = false;
      requested_donor_vin = false;
      donor_vin_initialized = true;
      sprintf(serial_debug_string, "Received HU donor VIN: %c%c%c%c%c%c%c.", DONOR_VIN[0], DONOR_VIN[1], DONOR_VIN[2],
                                    DONOR_VIN[3], DONOR_VIN[4], DONOR_VIN[5], DONOR_VIN[6]);
      serial_log(serial_debug_string, 3);
    }
  }
}


void evaluate_faceplate_buttons(void) {
  if (!digitalRead(FACEPLATE_EJECT_PIN)) {
    if (!faceplate_eject_pressed) {
      if (faceplate_eject_debounce_timer >= 1000) {
        faceplate_eject_pressed = true;
        faceplate_eject_pressed_timer = 0;
      }
    }
  } else {
    if (faceplate_eject_pressed) {
      if (faceplate_eject_pressed_timer >= 20                                                                                       // These inputs are very noisy. Make sure they're actually pressed.
          && !requested_hu_off_t2 && !hu_bn2000_bus_sleep_active) {                                                                 // Faceplate inputs should be ignored if the HU is to sleep.
        unsigned long time_now = millis();
        m = {faceplate_eject_buf, time_now};
        faceplate_buttons_txq.push(&m);
        time_now += 200;
        m = {faceplate_a1_released_buf, time_now};
        faceplate_buttons_txq.push(&m);
        serial_log("Faceplate eject button pressed.", 2);
        faceplate_eject_debounce_timer = 0;
      }
      faceplate_eject_pressed = false;
    }
  }
  
  if (!requested_hu_off_t2 && !hu_bn2000_bus_sleep_active) {                                                                        // 30G is about to be shut OFF. HU must now sleep without interruption.
    if (!digitalRead(FACEPLATE_POWER_MUTE_PIN)) {
      if (!faceplate_power_mute_pressed) {
        if (faceplate_power_mute_debounce_timer >= 150) {
          faceplate_power_mute_pressed = true;
          faceplate_power_mute_pressed_timer = 0;
        }
      } else {
        if (faceplate_power_mute_pressed_timer >= 8000) {
          if (!faceplate_hu_reboot) {
            if (diag_transmit) {
              serial_log("Faceplate power/mute button held. Rebooting HU and faceplate.", 2);
              send_cc_message("iDrive will reboot. Release button.", true, 3000);
              unsigned long time_now = millis() + 2000;
              m = {f_hu_nbt_reboot_buf, time_now};
              faceplate_buttons_txq.push(&m);
              time_now += 200;
              m = {f_hu_nbt_reboot_buf, time_now};
              faceplate_buttons_txq.push(&m);
              faceplate_hu_reboot = true;                                                                                           // Will ignore the next normal button release action (single press).
              faceplate_reset();
            }
          }
        }
      }
    } else {
      if (faceplate_power_mute_pressed) {
        if (!faceplate_hu_reboot) {
          if (faceplate_power_mute_pressed_timer >= 20 && !requested_hu_off_t2) {                                                   // With 30G kill imminent, do not allow the HU to be restarted.
            unsigned long time_now = millis();
            m = {faceplate_power_mute_buf, time_now};
            faceplate_buttons_txq.push(&m);
            time_now += 100;
            m = {faceplate_a1_released_buf, time_now};
            faceplate_buttons_txq.push(&m);
            serial_log("Faceplate power/mute button pressed.", 2);
          }
        } else {
          faceplate_hu_reboot = false;
        }
        faceplate_power_mute_debounce_timer = 0;
        faceplate_power_mute_pressed = false;
      }
    }
  }
}


void check_faceplate_buttons_queue(void) {
  if (!faceplate_buttons_txq.isEmpty()) {
    faceplate_buttons_txq.peek(&delayed_tx);
    if (millis() >= delayed_tx.transmit_time) {
      kcan2_write_msg(delayed_tx.tx_msg);
      faceplate_buttons_txq.drop();
      if (delayed_tx.tx_msg.id == 0x6F1 && delayed_tx.tx_msg.buf[5] == 1) {
        FACEPLATE_UART.begin(38400, SERIAL_8E1);
        FACEPLATE_UART.clear();
      }
    }
  }
}


void evaluate_faceplate_uart(void) {
  uint8_t faceplate_status_message[] = {0, 0, 0, 0, 0, 0, 0};

  if (vehicle_awakened_timer >= 1000) {
    if (FACEPLATE_UART.available() > 6) {                                                                                           // Wait for the full message to come in. Cycle time 1s.

      if (FACEPLATE_UART.read() == 0x55 && FACEPLATE_UART.read() == 0xAA) {
        // Serial.print("Faceplate: ");
        for (uint8_t i = 2; i < 7; i++) {
          faceplate_status_message[i] = FACEPLATE_UART.read();
          // Serial.print(faceplate_status_message[i], HEX); Serial.print(" ");
        }
        // Serial.println();

        if (faceplate_status_message[2] == 0x80) {                                                                                  // Seeking right
          kcan2_write_msg(faceplate_seek_right_buf);
        } else if (faceplate_status_message[2] == 0x40) {                                                                           // Seeking left
          kcan2_write_msg(faceplate_seek_left_buf);
        } else {                                                                                                                    // Released
          kcan2_write_msg(faceplate_a3_released_buf);
        }
        uint16_t faceplate_new_volume = (faceplate_status_message[6] << 8) | faceplate_status_message[4];
        if (faceplate_new_volume != faceplate_volume) {
          int16_t volume_difference = faceplate_new_volume - faceplate_volume;

          if (volume_difference > 0 && volume_difference <= 0x7FFF) {                                                               // Clockwise rotation
            kcan2_write_msg(faceplate_volume_decrease_buf);
          } else if (volume_difference < 0 && volume_difference >= -0x7FFF) {                                                       // Counterclockwise rotation
            kcan2_write_msg(faceplate_volume_increase_buf);
          } else {                                                                                                                  // Handle wraparound
            if (volume_difference > 0) {
                kcan2_write_msg(faceplate_volume_decrease_buf);
            } else {
                kcan2_write_msg(faceplate_volume_increase_buf);
            }
          }

          faceplate_volume = faceplate_new_volume;
        } else {
            kcan2_write_msg(faceplate_f1_released_buf);
        }

        if (faceplate_status_message[3] == 0x80) {
          if (faceplate_status_message[5] == 0x80) {
            kcan2_write_msg(faceplate_button1_press_buf);
          } else {
            kcan2_write_msg(faceplate_button1_hover_buf);
          }
        } else if (faceplate_status_message[3] == 0x40) {
          if (faceplate_status_message[5] == 0x40) {
            kcan2_write_msg(faceplate_button2_press_buf);
          } else {
            kcan2_write_msg(faceplate_button2_hover_buf);
          }
        } else if (faceplate_status_message[3] == 0x20) {
          if (faceplate_status_message[5] == 0x20) {
            kcan2_write_msg(faceplate_button3_press_buf);
          } else {
            kcan2_write_msg(faceplate_button3_hover_buf);
          }
        } else if (faceplate_status_message[3] == 0x10) {
          if (faceplate_status_message[5] == 0x10) {
            kcan2_write_msg(faceplate_button4_press_buf);
          } else {
            kcan2_write_msg(faceplate_button4_hover_buf);
          }
        } else if (faceplate_status_message[3] == 8) {
          if (faceplate_status_message[5] == 8) {
            kcan2_write_msg(faceplate_button5_press_buf);
          } else {
            kcan2_write_msg(faceplate_button5_hover_buf);
          }
        } else if (faceplate_status_message[3] == 4) {
          if (faceplate_status_message[5] == 4) {
            kcan2_write_msg(faceplate_button6_press_buf);
          } else {
            kcan2_write_msg(faceplate_button6_hover_buf);
          }
        } else if (faceplate_status_message[3] == 2) {
          if (faceplate_status_message[5] == 2) {
            kcan2_write_msg(faceplate_button7_press_buf);
          } else {
            kcan2_write_msg(faceplate_button7_hover_buf);
          }
        } else if (faceplate_status_message[3] == 1) {
          if (faceplate_status_message[5] == 1) {
            kcan2_write_msg(faceplate_button8_press_buf);
          } else {
            kcan2_write_msg(faceplate_button8_hover_buf);
          }
        } else {
          kcan2_write_msg(faceplate_a2_released_buf);
        }
      } else {                                                                                                                      // Faceplate is in some error state / message is out of sync.
        return;
      }

      faceplate_uart_watchdog_timer = 0;
      faceplate_reset_counter = 0;
    }
  } else {
    FACEPLATE_UART.clear();                                                                                                         // This data is garbage. Clear the RX buffer.
  }
}


void faceplate_reset(void) {                                                                                                        // The hardware reset pin on the faceplate could be used instead.
  if (diag_transmit) {
    FACEPLATE_UART.clear();
    FACEPLATE_UART.end(); 
    kcan_write_msg(ihka_5v_off_buf);
    unsigned long time_now = millis();
    m = {ihka_5v_on_buf, time_now + 300};                                                                                           // By cutting off 5V the uC of the faceplate will be reset.
    faceplate_buttons_txq.push(&m);
  }
}


void check_faceplate_watchdog(void) {
  #if F_NBTE
    if (!kl30g_cutoff_imminent && kcan2_mode == MCP_NORMAL && vehicle_awakened_timer >= 5000) {
      if (faceplate_uart_watchdog_timer >= 30000) {
        if (faceplate_reset_counter < 5) {
          serial_log("Faceplate stopped responding. Attempting reset.", 2);
          faceplate_reset();
          faceplate_uart_watchdog_timer = 0;
          faceplate_reset_counter++;
        }
      }
    } else {
      faceplate_uart_watchdog_timer = 0;
    }
  #endif
}


void convert_zbe1_message(void) {
  #if F_NBTE_CCC_ZBE
    zbe_action_counter = (zbe_action_counter + 1) % 0x100;                                                                          // Increase counter and reset when past 0xFF.
    zbe_buttons[2] = zbe_action_counter;

    uint16_t current_position = k_msg.buf[3] << 8 | k_msg.buf[2];                                                                   // Correct ZBE counter starting at 1 instead of 0.
    current_position = (current_position - 1) % 0x10000;
    k_msg.buf[2] = current_position & 0xFF;
    k_msg.buf[3] = current_position >> 8;

    if (zbe_rotation[3] != (k_msg.buf[2])) {
      zbe_rotation[2] = zbe_action_counter;
      zbe_rotation[3] = k_msg.buf[2];
      zbe_rotation[4] = (0x80 + k_msg.buf[3]) % 0x100;
      kcan2_write_msg(make_msg_buf(0x264, 6, zbe_rotation));
      return;
    }

    if (k_msg.buf[1] == 0xC4 || k_msg.buf[1] == 0xC5) {                                                                             // MENU
      zbe_buttons[3] = 1;
      zbe_buttons[4] = 0xC0;
      kcan2_write_msg(make_msg_buf(0x267, 6, zbe_buttons));
    } else if (k_msg.buf[0] == 0 && k_msg.buf[1] == 0xC0) {                                                                         // Joystick north
      zbe_buttons[3] = 0x11;
      zbe_buttons[4] = 0xDD;
      kcan2_write_msg(make_msg_buf(0x267, 6, zbe_buttons));
    } else if (k_msg.buf[0] == 4 && k_msg.buf[1] == 0xC0) {                                                                         // Joystick south
      zbe_buttons[3] = 0x41;
      zbe_buttons[4] = 0xDD;
      kcan2_write_msg(make_msg_buf(0x267, 6, zbe_buttons));
    } else if (k_msg.buf[0] == 6 && k_msg.buf[1] == 0xC0) {                                                                         // Joystick east
      zbe_buttons[3] = 0x81;
      zbe_buttons[4] = 0xDD;
      kcan2_write_msg(make_msg_buf(0x267, 6, zbe_buttons));
    } else if (k_msg.buf[0] == 2 && k_msg.buf[1] == 0xC0) {                                                                         // Joystick west
      zbe_buttons[3] = 0x21;
      zbe_buttons[4] = 0xDD;
      kcan2_write_msg(make_msg_buf(0x267, 6, zbe_buttons));
    } else if (k_msg.buf[1] == 0xC1) {                                                                                              // Joystick pressed
      zbe_buttons[3] = 1;
      zbe_buttons[4] = 0xDE;
      kcan2_write_msg(make_msg_buf(0x267, 6, zbe_buttons));
    } else {                                                                                                                        // Released
      zbe_buttons[3] = 0;
      kcan2_write_msg(make_msg_buf(0x267, 6, zbe_buttons));
    }
  #endif
}


void evaluate_cc_gong_status(void) {
  if (k_msg.buf[0] != 0xF0) {
    gong_active = true;
  } else {
    gong_active = false;
  }
}


void evaluate_idrive_units(void) {
  uint8_t new_language = k_msg.buf[0], new_temperature_unit = 0, new_time_format = 0,
          new_distance_unit = 0, new_consumption_unit = 0, new_date_format = 0,
          new_pressure_unit = 0, new_torque_unit = k_msg.buf[4] >> 4, new_power_unit = k_msg.buf[4] & 0xF;
  char language_str[4];

  bitWrite(new_temperature_unit, 0, bitRead(k_msg.buf[1], 4));
  bitWrite(new_temperature_unit, 1, bitRead(k_msg.buf[1], 5));

  bitWrite(new_time_format, 0, bitRead(k_msg.buf[1], 2));
  bitWrite(new_time_format, 1, bitRead(k_msg.buf[1], 3));

  bitWrite(new_distance_unit, 0, bitRead(k_msg.buf[2], 6));
  bitWrite(new_distance_unit, 1, bitRead(k_msg.buf[2], 7));

  bitWrite(new_consumption_unit, 0, bitRead(k_msg.buf[2], 0));
  bitWrite(new_consumption_unit, 1, bitRead(k_msg.buf[2], 1));
  bitWrite(new_consumption_unit, 2, bitRead(k_msg.buf[2], 2));

  bitWrite(new_date_format, 0, bitRead(k_msg.buf[3], 3));
  bitWrite(new_date_format, 1, bitRead(k_msg.buf[3], 4));
  bitWrite(new_date_format, 2, bitRead(k_msg.buf[3], 5));

  bitWrite(new_pressure_unit, 0, bitRead(k_msg.buf[3], 0));
  bitWrite(new_pressure_unit, 1, bitRead(k_msg.buf[3], 1));

  if (new_language > 0) {
    snprintf(language_str, sizeof(language_str), "%u", new_language);
    uint8_t idrive_bn2000_language[] = {0x1E, 0x66, 0, 1, new_language, 0, 0, 0};
    kcan_write_msg(make_msg_buf(0x5E2, 8, idrive_bn2000_language));                                                                 // KOMBI87 does not support/store all EVO languages. Check "coding parameters.txt" for fix.
  }

  if (new_temperature_unit == 1) {
    kcan_write_msg(idrive_bn2000_temperature_c_buf);
  } else if (new_temperature_unit == 2) {
    kcan_write_msg(idrive_bn2000_temperature_f_buf);
  }
  
  if (new_time_format == 1) {
    kcan_write_msg(idrive_bn2000_time_12h_buf);
  } else if (new_time_format == 2) {
    kcan_write_msg(idrive_bn2000_time_24h_buf);
  }

  if (new_distance_unit == 1) {
    kcan_write_msg(idrive_bn2000_distance_km_buf);
  } else if (new_distance_unit == 2) {
    kcan_write_msg(idrive_bn2000_distance_mi_buf);
  }

  if (new_consumption_unit == 1) {
    kcan_write_msg(idrive_bn2000_consumption_l100km_buf);
  } else if (new_consumption_unit == 2) {
    kcan_write_msg(idrive_bn2000_consumption_mpguk_buf);
  } else if (new_consumption_unit == 3) {
    kcan_write_msg(idrive_bn2000_consumption_mpgus_buf);
  } else if (new_consumption_unit == 4) {
    kcan_write_msg(idrive_bn2000_consumption_kml_buf);
  }

  if (new_pressure_unit > 0) {
    // if (new_pressure_unit == 1) {
    //   kcan_write_msg(idrive_bn2000_pressure_bar_buf);                                                                               // Despite this functionality existing in CIC, KOMBI does not save these.
    // } else if (new_pressure_unit == 2) {
    //   kcan_write_msg(idrive_bn2000_pressure_kpa_buf);
    // } else if (new_pressure_unit == 3) {
    //   kcan_write_msg(idrive_bn2000_pressure_psi_buf);
    // }

    bitWrite(pressure_unit_date_format[cas_key_number], 0, bitRead(new_pressure_unit, 0));
    bitWrite(pressure_unit_date_format[cas_key_number], 1, bitRead(new_pressure_unit, 1));
  }

  if (new_date_format > 0) {
    if (new_date_format == 1) {
      kcan_write_msg(idrive_bn2000_date_ddmmyyyy_buf);
    } else if (new_date_format == 2) {
      kcan_write_msg(idrive_bn2000_date_mmddyyyy_buf);
    } 
    // Extended date formats (yyyy/mm/dd and yyyy.mm.dd) not supported by the KOMBI. 
    // They are stored in the EEPROM instead for use by the HU only.

    bitWrite(pressure_unit_date_format[cas_key_number], 3, bitRead(new_date_format, 0));
    bitWrite(pressure_unit_date_format[cas_key_number], 4, bitRead(new_date_format, 1));
    bitWrite(pressure_unit_date_format[cas_key_number], 5, bitRead(new_date_format, 2));
  }
  
  if (new_torque_unit > 0) {
    torque_unit[cas_key_number] = new_torque_unit;
  }
  
  if (new_power_unit > 0) {
    power_unit[cas_key_number] = new_power_unit;
  }

  sprintf(serial_debug_string,
          "Received iDrive settings: Language:%s Temperature:%s Time:%s Distance:%s Consumption:%s Pressure:%s Date:%s Torque:%s Power:%s.",
          new_language > 0 ? language_str : "-",
          new_temperature_unit == 1 ? "C" : (new_temperature_unit == 2 ? "F" : "-"),
          new_time_format == 1 ? "12h" : (new_time_format == 2 ? "24h" : "-"),
          new_distance_unit == 1 ? "km" : (new_distance_unit == 2 ? "mi" : "-"),
          new_consumption_unit == 1 ? "l/100km" 
                                    : (new_consumption_unit == 2 ? "mpg-uk"
                                    : (new_consumption_unit == 3 ? "mpg-us"
                                    : (new_consumption_unit == 4 ? "km/l"
                                    : "-"))),
          new_pressure_unit == 1 ? "bar"
                                 : (new_pressure_unit == 2 ? "kPa"
                                 : (new_pressure_unit == 3 ? "psi" : "-")),
          new_date_format == 1 ? "dd.mm.yyyy" 
                               : (new_date_format == 2 ? "mm/dd/yyyy"
                               : (new_date_format == 6 ? "yyyy/mm/dd"
                               : (new_date_format == 5 ? "yyyy.mm.dd"
                               : "-"))),
          new_torque_unit == 1 ? "Nm" : (new_torque_unit == 2 ? "lb-ft" : (new_torque_unit == 3 ? "Kg-m" : "-")),
          new_power_unit == 1 ? "kW" : (new_power_unit == 2 ? "hp" : "-"));
  serial_log(serial_debug_string, 3);
  convert_f_units(true);
  send_nbt_sport_displays_data(false);                                                                                              // Update the scale for the new units.
}


void convert_f_units(bool idrive_units_only) {
  if (!idrive_units_only) {
    f_units[0] = k_msg.buf[0];
    f_units[1] = k_msg.buf[1];
    f_units[2] = k_msg.buf[2];
  }
  f_units[3] = pressure_unit_date_format[cas_key_number];
  f_units[4] = (torque_unit[cas_key_number] << 4) | power_unit[cas_key_number];
  kcan2_write_msg(make_msg_buf(0x2F7, 6, f_units));
}


// NOTE: Max input to this function is 45 characters + NUL terminator.
void send_cc_message(const char input[], bool dialog, unsigned long new_timeout) {
  nbt_cc_txq.flush();                                                                                                               // Clear any pending dismiss messages.
  uint8_t input_length = strlen(input);                                                                                             // Length excluding NUL terminator!
  if (input_length > 45) {
    serial_log("String length exceeded for send_cc_message.", 1);
    return;
  }
  uint8_t padding = (input_length % 3 == 0) ? 0 : 3 - (input_length % 3);
  uint8_t padded_length = input_length + padding;
  char padded_input[padded_length];
  memset(padded_input, ' ', padded_length);
  memcpy(padded_input, input, input_length);
  
  uint8_t cc_message_chunk_counter = (((padded_length / 3) - 2) << 4) | 0xF;

  for (uint8_t i = 0; i < (padded_length / 3); i++) {
    cc_message_chunk_counter = (cc_message_chunk_counter + 1) % 0x100;
    uint8_t cc_message_text[] = {0x46, 3, 0x50, 0xF0, cc_message_chunk_counter,
                                padded_input[3 * i], padded_input[(3 * i) + 1], padded_input[(3 * i) + 2]};
    
    if (dialog) {
      cc_message_text[2] = 0x32;
    }
    
    kcan2_write_msg(make_msg_buf(0x338, 8, cc_message_text));
  }
  
  cc_message_chunk_counter = (cc_message_chunk_counter + 1) % 0x100;
  uint8_t cc_message_text_end[] = {0x46, 3, 0x50, 0xF0, cc_message_chunk_counter, 0x20, 0x20, 0x20};
  
  if (dialog) {
    cc_message_text_end[2] = 0x32;
    cc_message_expires = millis() + new_timeout;                                                                                    // Do not replace this message with the monitoring CC until the timeout elapses.
    m = {custom_cc_dismiss_buf, cc_message_expires};
    nbt_cc_txq.push(&m);
  }
  
  kcan2_write_msg(make_msg_buf(0x338, 8, cc_message_text_end));
  custom_info_cc_timer = 3000;
}


void check_nbt_cc_queue(void) {
  if (!nbt_cc_txq.isEmpty()) {
    nbt_cc_txq.peek(&delayed_tx);
    if (millis() >= delayed_tx.transmit_time) {
      kcan2_write_msg(delayed_tx.tx_msg);
      nbt_cc_txq.drop();
    }
  }
}


void evaluate_idrive_zero_time(void) {
  if (k_msg.buf[0] == 0 && k_msg.buf[1] == 0 && k_msg.buf[3] == 1 
      && k_msg.buf[4] == 0x1F && k_msg.buf[5] == 0xD0 && k_msg.buf[6] == 7) {
    return;
  } else {
    kcan_write_msg(k_msg);
  }
}


void evaluate_idrive_lights_settings(void) {                                                                                        // Force light settings through a parser that forwards to KOMBI.
  if ((k_msg.buf[0] >> 4) == 9) {
    kcan_write_msg(idrive_bn2000_indicator_single_buf);
  } else if ((k_msg.buf[0] >> 4) == 0xA) {
    kcan_write_msg(idrive_bn2000_indicator_triple_buf);
  } else if (k_msg.buf[1] == 0xFF && k_msg.buf[2] == 0xF1) {
    kcan_write_msg(idrive_bn2000_drl_off_buf);
  } else if (k_msg.buf[1] == 0xFF && k_msg.buf[2] == 0xF2) {
    kcan_write_msg(idrive_bn2000_drl_on_buf);
  } else if (k_msg.buf[2] == 0xF4) {
    uint8_t home_lights_setting[] = {0x1E, 0x48, 0, 1, 0, k_msg.buf[1], 0, 0};
    kcan_write_msg(make_msg_buf(0x5E2, 8, home_lights_setting));
    f_lights_ckm_request = k_msg.buf[2];
  }
}


void evaluate_consumer_control(void) {
  special_consumers_power = constrain(k_msg.buf[1] * 0.5, 0, 100.0);                                                                // CTR_PWR_SPCOS
  #if F_NBTE
  // NOTE: if 0x3B3 is not sent to the HU it cannot be woken with the faceplate power button after Terminal R OFF!

  // BN2000 0x3B3 is missing ST_ENERG_PWMG and CTR_PWRU_COS is always 0xF.
  uint8_t f_consumer_control[] = {k_msg.buf[0], k_msg.buf[1], k_msg.buf[2],                                                         // 6MC2DL0B pg. 1391.
                                  k_msg.buf[3], k_msg.buf[4], k_msg.buf[5],
                                  0xF8};
  if (!requested_hu_off_t2 && !hu_bn2000_bus_sleep_active) {
    kcan2_write_msg(make_msg_buf(0x3B3, 7, f_consumer_control));
  } else {
    // With 30G cutoff happening soon, ensure that only OFF messages are sent.
    // OR
    // If BN2000(OSEK) HU NM set to sleep, request KCAN2 consumers to go OFF.
    
    f_consumer_control[5] = 8;                                                                                                      // CTR_PCOS - 2 - stationary_consumers_must_switch_off.
    f_consumer_control[6] = 0xF2;                                                                                                   // ST_ENERG_PWMG - 2 - No_stationary_consumers_permitted.
    kcan2_write_msg(make_msg_buf(0x3B3, 7, f_consumer_control));
  }
  #endif
}


void evaluate_speed_warning_status(void) {
  uint16_t speed_warning_status = (k_msg.buf[2] << 4) | (k_msg.buf[1] >> 4);

  if (speed_warning_status < 0xF) {                                                                                                 // If less than 15kph, force to 15.
    if ((k_msg.buf[1] && 0xF) == 1) {
      kcan_write_msg(set_warning_15kph_on_buf);                                                                                     // The status response should then fix the HU setting.
    } else {
      kcan_write_msg(set_warning_15kph_off_buf);
    }
  } else if (speed_warning_status == 0xFFF) {                                                                                       // Invalid state.
    kcan_write_msg(set_warning_15kph_off_buf);
  } else {
    kcan2_write_msg(k_msg);
  }
}


void evaluate_speed_warning_setting(void) {
  if (k_msg.buf[0] == 0x6F) {
    k_msg.buf[0] = 0x7E;
  } else if (k_msg.buf[0] == 0x2F) {
    k_msg.buf[0] = 0x3E;
  }
  kcan_write_msg(k_msg);
}


void send_f_lcd_brightness(void) {
  uint8_t DSTN_LCD_LUM = constrain(k_msg.buf[2], 0x3F, 0xFE);
  uint8_t f_lcd_brightness[] = {DSTN_LCD_LUM,
                                (uint8_t)constrain(k_msg.buf[1], 0, 0xF0),                                                          // DMPNG_LCD_LUM
                                0,
                                0xFD};                                                                                              // DAY/NIGHT mode.

  if (rls_time_of_day >= 2 && DSTN_LCD_LUM <= 0xB2) {                                                                               // If LCD brightness goes over 70%, switch off night mode.
    f_lcd_brightness[3] = 0xFE;                                                                                                     // This makes the HU switch to night mode.
  } else if (rls_time_of_day == 1) {
    f_lcd_brightness[0] = constrain(f_lcd_brightness[0] + 0x19, 0, 0xFE);                                                           // Increased slightly (10%) to bias CID brightness during dusk/twilight.
  } else if (rls_time_of_day == 0) {
    f_lcd_brightness[0] = constrain(f_lcd_brightness[0] + 0x32, 0, 0xFE);                                                           // Increased more (20%) to bias CID brightness during the day.
  }
  
  kcan2_write_msg(make_msg_buf(0x393, 4, f_lcd_brightness));
}


void send_f_interior_ambient_light_brightness(void) {
  uint8_t f_interior_ambient_light_brightness[] = {0xFF, 0xFF, 0xFF, 0, 0x1F, 0xFF, 0xFF, 0xFF};
  switch (k_msg.buf[0]) {
    case 0:
      f_interior_ambient_light_brightness[3] = 0x1F;
      break;
    case 0x1C:
      f_interior_ambient_light_brightness[3] = 0x37;
      break;
    case 0x38:
      f_interior_ambient_light_brightness[3] = 0x4F;
      break;
    case 0x54:
      f_interior_ambient_light_brightness[3] = 0x67;
      break;
    case 0x70:
      f_interior_ambient_light_brightness[3] = 0x7F;
      break;
    case 0x8D:
      f_interior_ambient_light_brightness[3] = 0x97;
      break;
    case 0xA9:
      f_interior_ambient_light_brightness[3] = 0xAF;
      break;
    case 0xC5:
      f_interior_ambient_light_brightness[3] = 0xC7;
      break;
    case 0xE1:
      f_interior_ambient_light_brightness[3] = 0xDF;
      break;
    case 0xFD:
      f_interior_ambient_light_brightness[3] = 0xFE;
      break;
  }
  kcan2_write_msg(make_msg_buf(0x45C, 8, f_interior_ambient_light_brightness));
}


void process_hu_kombi_settings(void) {
  if (terminal_r) {
    if (k_msg.buf[0] == 0x1E || k_msg.buf[0] == 0x11) {                                                                             // Only forward this message if it is actually relevant for the BN2000 KOMBI.
      kcan_write_msg(k_msg);
    }
  }
}
