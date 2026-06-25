# Integration

## Module Description
End-to-end integration scripts and system-level tests that wire all FSM modules together into a running Smart Campus Parking System. This is the single entry point for full system bring-up and cross-module validation.


## Language Used
[e.g. Python / Shell]

## Hardware/Device
All connected hardware (see individual module READMEs)

## Sensors/Components
All sensors and components from all modules (see individual READMEs)

---

## Directory Structure
```
integration/
├── src/     # Integration entry-point scripts and system test suites
└── docs/    # Integration test plans, system wiring overview
```

---

## Interface/Communication
This module does not implement an FSM of its own. It orchestrates communication between all modules:

| Module A | → | Module B | Signal / Data |
|----------|---|----------|---------------|
| entrance-gate-module | → | space-management-counter | INCREMENT pulse on vehicle entry |
| exit-gate-module | → | space-management-counter | DECREMENT pulse on vehicle exit |
| space-management-counter | → | entrance-gate-module | FULL signal when lot is at capacity |
| space-management-counter | → | adaptive-lighting-efficiency | Occupancy level for dimming decisions |
| traffic-congestion-prevention | → | entrance-gate-module | STOP signal when queue threshold exceeded |
| safety-emergency-override | → | ALL modules | EMERGENCY broadcast — forces gates open, full lighting |
| ALL modules | → | iot-dashboard-hps | Event data stream for logging and display |

---

## How to Run/Build

### Prerequisites
All individual modules must be set up first. Refer to each module's README.md.

### Run Full System
```bash
python integration/src/main.py
```

### Run Integration Tests
```bash
python integration/src/test_integration.py
```
