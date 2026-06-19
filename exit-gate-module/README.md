# Exit Gate Module

## Module Description
Controls the physical exit gate of the parking lot. Detects when a vehicle is ready to leave, opens the gate to allow passage, holds it open until the vehicle clears, then closes and stops the motor.

---

## Assigned Member
[NAME]

## Language Used
[e.g. SystemVerilog / Python / C++]

## Hardware/Device
[e.g. FPGA DE10-Nano / Arduino Uno / Raspberry Pi]

## Sensors/Components
[list here — e.g. IR/ultrasonic exit sensor, servo motor, loop detector]

---

## FSM States

| State | Description |
|-------|-------------|
| `IDLE` | Waiting for a vehicle to approach the exit lane |
| `ALLOW_EXIT` | Vehicle detected at exit — authorize gate to open (no auth required) |
| `OPEN_GATE` | Send open command to gate motor controller |
| `HOLD_GATE` | Keep gate open while vehicle clears the exit sensor |
| `CLOSE_GATE` | Send close command once vehicle has fully passed through |
| `TERMINATE_MOTOR` | Stop motor drive signal after gate reaches closed position |

### State Transition Diagram
```
IDLE ──[vehicle detected]──► ALLOW_EXIT ──► OPEN_GATE
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
[how this module talks to others — e.g., sends DECREMENT pulse to space-management-counter on ALLOW_EXIT; notifies iot-dashboard-hps of exit events over UART/SPI]

---

## How to Run/Build
[instructions here]
