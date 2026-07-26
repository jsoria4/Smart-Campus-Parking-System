# Entrance Gate Module

## Module Description
Controls the physical entry gate of the parking lot. Uses RFID/NFC tag detection to authenticate vehicles, interfaces with the space management counter to check availability, and drives the gate motor through its open/hold/close cycle.

---

## Assigned Member
Landon

## Language Used
C++

## Hardware/Device
Arduino Uno

## Sensors/Components
[list here — e.g. RFID reader, servo motor, IR sensor, LED indicators]
Small Servo Motor
RFID Reader
LED indicators
Passive Buzzer
Ultrasonic Sensor

---

## FSM States

| State | Description |
|-------|-------------|
| `IDLE` | Waiting for a vehicle/tag to be presented at the entrance |
| `TAG_DETECTED` | RFID/NFC tag signal received — begin verification process |
| `VERIFY_TAG` | Validate tag ID against the authorized vehicle database |
| `ACCESS_GRANTED` | Tag is valid and a space is available — proceed to open gate |
| `ACCESS_DENIED` | Tag is invalid or lot is full — signal rejection to driver |
| `OPEN_GATE` | Send open command to gate motor controller |
| `HOLD_GATE` | Keep gate open while vehicle clears the sensor |
| `CLOSE_GATE` | Send close command once vehicle has passed through |
| `TERMINATE_MOTOR` | Stop motor drive signal after gate reaches closed position |

### State Transition Diagram
```
IDLE ──[tag detected]──► TAG_DETECTED ──► VERIFY_TAG
                                               │
                          ┌────────────────────┴──────────────────┐
                    [valid + space]                          [invalid / full]
                          │                                        │
                          ▼                                        ▼
                   ACCESS_GRANTED                           ACCESS_DENIED
                          │                                        │
                          ▼                                        ▼
                      OPEN_GATE                                  IDLE
                          │
                          ▼
                      HOLD_GATE ──[vehicle cleared]──► CLOSE_GATE
                                                            │
                                                            ▼
                                                     TERMINATE_MOTOR
                                                            │
                                                            ▼
                                                           IDLE
```

---

## Interface/Communication
(output) vehicle_entered -> A brief signal sent to FSM #1 Space Management & Controller to increment the vehicle count
(input) emergency_override -> A signal from FSM #3 Safety & Emergency Override to force the gate open for emergency purposes
(input) plate_authorized -> A signal from the Raspberry Pi 5 AI Model confirming a plate was read
(input) space_full -> A signal from FSM #1 incidating if there are no more parking spots available

---

## How to Run/Build
1. Open EntranceFSM.ino in src folder in Arduino IDE.
2. Connect an Arduino Uno to your device via USB
3. Press the arrow in the top left to deploy the code to the Microcontroller
4. Done!