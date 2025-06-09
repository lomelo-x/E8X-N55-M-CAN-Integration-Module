# Wiring Diagram for E82 N55 M Button Module

## CAN Bus Connections

### K-CAN (Cluster/Interior - 100kbps)

```
Teensy 4.0/4.1      CAN Transceiver
Pin 22 -----------> CAN Low
Pin 23 -----------> CAN High
```

Connect to vehicle K-CAN:

- K-CAN Low (Green wire)
- K-CAN High (Orange/Green wire)

### PT-CAN (Engine/DME - 500kbps)

```
Teensy 4.0/4.1      CAN Transceiver
Pin 0 ------------> CAN Low
Pin 1 ------------> CAN High
```

Connect to vehicle PT-CAN:

- PT-CAN Low (Red/Black wire)
- PT-CAN High (Blue/Red wire)

## Power Supply

### Teensy Power

```
Vehicle Power       Teensy 4.0/4.1
12V (Fused) -----> 5V Regulator -----> VIN
Ground ----------> GND
```

Notes:

- Use a 5V regulator rated for at least 500mA
- Add a 1-2A fuse on the 12V input
- Use proper gauge wire for power (20-22 AWG recommended)

## CAN Transceiver Power

### K-CAN Transceiver

```
Teensy 4.0/4.1      CAN Transceiver
3.3V ------------> VDD
GND -------------> GND
```

### PT-CAN Transceiver

```
Teensy 4.0/4.1      CAN Transceiver
3.3V ------------> VDD
GND -------------> GND
```

## Important Notes

1. CAN Bus Termination:

   - Both CAN buses should be properly terminated with 120Ω resistors
   - Vehicle typically provides termination on one end
   - Add termination resistor if needed on module end

2. Power Supply:

   - Use clean, stable power source
   - Add capacitors near voltage regulator:
     - 100µF electrolytic on input
     - 10µF ceramic on output

3. Grounding:

   - All grounds should be connected to a common point
   - Ensure good connection to vehicle chassis ground

4. Wire Gauge:

   - Power: 20-22 AWG
   - CAN Bus: 22-24 AWG twisted pair
   - Signal wires: 22-24 AWG

5. Protection:
   - Add TVS diodes on CAN lines for ESD protection
   - Use shielded cables for CAN connections
   - Heat shrink or insulate all connections

## Vehicle Connection Points

1. K-CAN:

   - Usually accessible at OBD2 port
   - Also found behind head unit

2. PT-CAN:

   - Available at DME connector
   - Also at OBD2 port

3. Power:
   - Use switched 12V source (on with ignition)
   - Can tap from fuse box with add-a-fuse

## Testing Procedure

1. Before connecting to vehicle:

   - Check all connections with multimeter
   - Verify no shorts between power and ground
   - Confirm CAN H and CAN L are not shorted

2. Initial power up:

   - Connect ground first
   - Monitor current draw
   - Check voltage at Teensy VIN pin

3. CAN communication:
   - Use serial monitor to verify messages
   - Check for proper termination resistance
   - Verify baud rates match vehicle
