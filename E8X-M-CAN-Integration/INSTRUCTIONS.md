# E82 N55 M Button Integration Module

This module provides M button functionality, gauge sweep animations, and MHD map switching for BMW E82 N55 vehicles.

## Hardware Requirements

1. Teensy 4.0/4.1 Microcontroller
2. 2x CAN transceivers:
   - K-CAN (100kbps) for cluster communication
   - PT-CAN (500kbps) for engine/DME communication

## Pin Connections

### CAN Bus Connections

- K-CAN (Cluster communication):

  - K-CAN High: Pin 23
  - K-CAN Low: Pin 22
  - Baud rate: 100kbps

- PT-CAN (Engine/DME):
  - PT-CAN High: Pin 1
  - PT-CAN Low: Pin 0
  - Baud rate: 500kbps

### M Button Integration

The M button functionality is handled entirely through CAN messages:

- Input: Message ID 0x1D9 (K-CAN)

  - Button Released: 0xFF
  - Button Pressed: 0xBF

- Output Messages:
  - M Mode Status: 0x0B6
  - DSC Control: 0x398
  - MHD Map Control: 0x6F1

## Features

1. M Button Functionality:

   - Short press: Toggle M mode on/off
   - When M mode is ON:
     - Illuminates M icon in cluster
     - Switches to MHD Map 1
     - Turns DSC off
   - When M mode is OFF:
     - Turns off M icon
     - Returns to stock MHD map
     - Turns DSC back on

2. Gauge Sweep:

   - Performs sweep animation on startup
   - Sweeps both speedometer and tachometer

3. MHD Map Integration:
   - Uses diagnostic messages (0x6F1)
   - Map 0 = Stock tune
   - Map 1 = Performance tune (activated with M button)

## CAN Message IDs

Important CAN IDs used:

- 0x1D9: M Button status
- 0x0B6: MDrive status (M icon)
- 0x6F1: Gauge control and MHD map switching
- 0x398: DSC status/control

## Installation

1. Install Required Libraries:

   - FlexCAN_T4 library for Teensy

2. Upload Code:

   - Upload all three files to your Teensy:
     - E8X_N55_M_Functions.ino
     - m_functions.h
     - m_functions.cpp

3. Wire Connections:
   - Connect CAN transceivers according to pin assignments
   - Power Teensy with stable 5V supply

## Troubleshooting

1. M Icon Not Illuminating:

   - Check K-CAN connections
   - Verify message 0x0B6 format

2. MHD Map Not Switching:

   - Check PT-CAN connections
   - Verify MHD is installed and working
   - Check diagnostic message format (0x6F1)

3. Gauge Sweep Not Working:
   - Verify K-CAN connections
   - Check message 0x6F1 format for gauge control

## Safety Notes

1. Always test in a safe environment
2. Be aware that DSC deactivation affects vehicle stability
3. Ensure proper CAN bus termination
4. Use appropriate fusing for power connections

## Support

For issues or questions:

1. Check connections and wiring
2. Verify CAN message formats
3. Monitor serial output (115200 baud) for debugging
