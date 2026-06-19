# Traffic Congestion Prevention

## Module Description
Monitors the vehicle queue at the parking lot entrance. When a queue is detected (vehicles backing up beyond a threshold), this module signals the entrance gate and external traffic systems to stop admitting vehicles, preventing congestion from spilling onto adjacent roads.

---

## Assigned Member
[NAME]

## Language Used
[e.g. SystemVerilog / Python / C++]

## Hardware/Device
[e.g. FPGA DE10-Nano / Arduino Uno / Raspberry Pi]

## Sensors/Components
[list here — e.g. IR/ultrasonic queue sensors, traffic signal controller interface, LED queue indicator]

---

## FSM States

| State | Description |
|-------|-------------|
| `IDLE` | No queue detected — normal traffic flow allowed |
| `QUEUE_DETECTED` | Queue length exceeds threshold — begin congestion response |
| `STOP` | Halt new vehicle admittance; signal external traffic system to redirect |

### State Transition Diagram
```
IDLE ──[queue length > threshold]──► QUEUE_DETECTED
 ▲                                         │
 │                                         ▼
 │                                        STOP
 │                                         │
 └─────────────[queue cleared]─────────────┘
```

---

## Interface/Communication
[how this module talks to others — e.g., receives space count from space-management-counter; sends STOP signal to entrance-gate-module to deny new entries; notifies iot-dashboard-hps to display congestion warning on dashboard]

---

## How to Run/Build
[instructions here]
