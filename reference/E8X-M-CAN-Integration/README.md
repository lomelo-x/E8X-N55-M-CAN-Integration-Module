# E82 N55 M Button CAN Integration Module

This project provides CAN bus integration for BMW E82 N55 M button functionality, including gauge sweep animations and MHD map switching.

## Project Structure

```
E8X-M-CAN-Integration/
├── E8X_N55_M_Functions.ino    # Main Arduino sketch
├── m_functions.cpp            # M button functionality implementation
├── m_functions.h             # M button function declarations
├── can_core.h               # CAN bus core functionality header
├── can_core.cpp            # CAN bus core implementation
├── src/                    # Required libraries
│   ├── FlexCAN_T4/        # Teensy CAN bus library
│   ├── CRC/               # CRC calculation library
│   └── queue/             # Queue implementation
├── INSTRUCTIONS.md        # Setup and usage instructions
├── WIRING.md             # Wiring diagrams and connections
└── README.md             # This file
```

## Dependencies

1. Hardware:

   - Teensy 4.0/4.1 microcontroller
   - CAN transceivers (for K-CAN and PT-CAN)

2. Libraries:
   - FlexCAN_T4 (included in src/)
   - CRC library (included in src/)
   - Queue library (included in src/)

## Installation

1. Copy all files to your Arduino project directory
2. Install the Teensy board support package in Arduino IDE
3. Select Teensy 4.0/4.1 as your board
4. Upload the sketch

## Usage

See INSTRUCTIONS.md for detailed setup and usage instructions.
See WIRING.md for connection diagrams and pinouts.

## License

MIT License - Feel free to use and modify as needed.
