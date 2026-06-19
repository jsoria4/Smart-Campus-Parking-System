# Smart Campus Parking System

A multi-hardware embedded systems project implementing a fully automated, FSM-driven parking lot management system for a university campus. Modules span FPGA, Arduino microcontrollers, and Python/HPS software, all connected through a shared integration layer.

## Team Members

| Name | Role | Module |
|------|------|--------|
| [Jasmine] | [Role] | Space Management Counter |
| [Landon] | [Role] | Entrance Gate Module |
| [Jeremy] | [Role] | Exit Gate Module |
| [Connor] | [Role] | Safety & Emergency Override |
| [Robert] | [Role] | Adaptive Lighting Efficiency |
| [Member 6] | [Role] | Traffic Congestion Prevention / IoT Dashboard |

---

## System Architecture Overview

Each functional unit is implemented as an independent Finite State Machine (FSM) module. Modules communicate through GPIO signals, UART, or SPI depending on hardware constraints.

```
                        ┌─────────────────────────────┐
                        │   safety-emergency-override │
                        │  (EMERGENCY broadcast →ALL) │
                        └──────────────┬──────────────┘
                                       │ interrupt / GPIO
          ┌────────────────────────────┼────────────────────────────┐
          │                            │                            │
          ▼                            ▼                            ▼
┌─────────────────┐          ┌─────────────────┐          ┌─────────────────┐
│ entrance-gate   │◄────────►│ space-mgmt-     │◄────────►│  exit-gate-     │
│ -module         │ FULL /   │ counter         │ DEC pulse │  module         │
│                 │ INC pulse│                 │           │                 │
└────────┬────────┘          └────────┬────────┘           └────────┬────────┘
         │                            │ occupancy level              │
         │◄── STOP signal             ▼                              │
┌────────┴────────┐          ┌─────────────────┐                    │
│ traffic-        │          │ adaptive-        │                    │
│ congestion-     │          │ lighting-        │                    │
│ prevention      │          │ efficiency       │                    │
└─────────────────┘          └─────────────────┘                    │
         │                            │                              │
         └────────────────────────────┼──────────────────────────────┘
                                      │ all event streams
                                      ▼
                          ┌───────────────────────┐
                          │   iot-dashboard-hps   │
                          │  (logging, display,   │
                          │   remote control)     │
                          └───────────────────────┘
                                      │
                                      ▼
                              ┌───────────────┐
                              │  integration/ │
                              │  (full system │
                              │   entry point)│
                              └───────────────┘
```

---

## Module Summary

| Module | FSM States | README |
|--------|-----------|--------|
| `space-management-counter` | IDLE, DECREMENT_COUNT, INCREMENT_COUNT, CHECK_CAPACITY, SPACE_FULL | [README](space-management-counter/README.md) |
| `entrance-gate-module` | IDLE, TAG_DETECTED, VERIFY_TAG, ACCESS_GRANTED, ACCESS_DENIED, OPEN_GATE, HOLD_GATE, CLOSE_GATE, TERMINATE_MOTOR | [README](entrance-gate-module/README.md) |
| `exit-gate-module` | IDLE, ALLOW_EXIT, OPEN_GATE, HOLD_GATE, CLOSE_GATE, TERMINATE_MOTOR | [README](exit-gate-module/README.md) |
| `safety-emergency-override` | MONITOR_ENVIRONMENT, EMERGENCY_DETECTED, ALARM_ACTIVE, EVACUATION_OVERRIDE, DISPLAY_EVAC | [README](safety-emergency-override/README.md) |
| `adaptive-lighting-efficiency` | MEASURE_LIGHT, DAY_MODE, NIGHT_MODE, SMART_DIMMING | [README](adaptive-lighting-efficiency/README.md) |
| `traffic-congestion-prevention` | IDLE, QUEUE_DETECTED, STOP | [README](traffic-congestion-prevention/README.md) |
| `iot-dashboard-hps` | SYNC_DATA, LOG_HISTORY, DASHBOARD_UPDATE, REMOTE_ADJUST | [README](iot-dashboard-hps/README.md) |
| `integration` | *(orchestration — no FSM)* | [README](integration/README.md) |

---

## Integration Instructions

1. Set up each module individually — refer to each module's `README.md` for hardware, language, and build instructions.
2. Connect hardware per the wiring diagrams in `docs/wiring-diagrams/`.
3. Ensure all serial ports are configured in `integration/src/config.json`.
4. Run the full system entry point:
   ```bash
   python integration/src/main.py
   ```
5. Open the IoT dashboard to monitor real-time occupancy and events.

---

## Repository Structure

```
.
├── space-management-counter/
│   ├── src/
│   ├── docs/
│   └── README.md
├── entrance-gate-module/
│   ├── src/
│   ├── docs/
│   └── README.md
├── exit-gate-module/
│   ├── src/
│   ├── docs/
│   └── README.md
├── safety-emergency-override/
│   ├── src/
│   ├── docs/
│   └── README.md
├── adaptive-lighting-efficiency/
│   ├── src/
│   ├── docs/
│   └── README.md
├── traffic-congestion-prevention/
│   ├── src/
│   ├── docs/
│   └── README.md
├── iot-dashboard-hps/
│   ├── src/
│   ├── docs/
│   └── README.md
├── integration/
│   ├── src/
│   ├── docs/
│   └── README.md
├── docs/                   # System-wide documentation, wiring diagrams, meeting notes
├── .gitignore
├── CONTRIBUTING.md
└── README.md
```

---

## Quick Links
- [Contributing Guidelines](CONTRIBUTING.md)
- [System-Wide Documentation](docs/README.md)
