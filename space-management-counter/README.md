# Space Management Counter

## Module Description
Tracks the real-time count of occupied and available parking spaces. Implements a finite state machine that increments or decrements the space counter as vehicles enter or exit, and signals when the lot reaches full capacity.

---

## Assigned Member
[NAME]

## Language Used
[e.g. SystemVerilog / Python / C++]

## Hardware/Device
[e.g. FPGA DE10-Nano / Arduino Uno / Raspberry Pi]

## Sensors/Components
[list here]

---

## FSM States

| State | Description |
|-------|-------------|
| `IDLE` | Waiting for an entry or exit event trigger |
| `DECREMENT_COUNT` | Vehicle has exited — decrease occupied count by 1 |
| `INCREMENT_COUNT` | Vehicle has entered — increase occupied count by 1 |
| `CHECK_CAPACITY` | Evaluate whether the lot has reached maximum capacity |
| `SPACE_FULL` | Lot is at full capacity — signal entrance gate to deny access |

### State Transition Diagram
```
         entry_event                    exit_event
IDLE ─────────────────► INCREMENT_COUNT          DECREMENT_COUNT ◄──── IDLE
                              │                        │
                              ▼                        ▼
                        CHECK_CAPACITY ◄──────── CHECK_CAPACITY
                              │
                    [count == max]│
                              ▼
                         SPACE_FULL ──────────────────► IDLE
                                      [reset/exit]
```

---

## Interface/Communication
[how this module talks to others — e.g., outputs a FULL signal to the entrance gate module over GPIO/UART; receives INCREMENT/DECREMENT pulses from entrance and exit gate modules]

---

## How to Run/Build
[instructions here]
