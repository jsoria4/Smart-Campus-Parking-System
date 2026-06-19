# Safety & Emergency Override

## Module Description
Continuously monitors environmental conditions (smoke, fire, gas, emergency button) and triggers a system-wide evacuation override when a hazard is detected. Overrides all gate and lighting modules, activates alarms, and displays evacuation instructions.

---

## Assigned Member
[NAME]

## Language Used
[e.g. SystemVerilog / Python / C++]

## Hardware/Device
[e.g. FPGA DE10-Nano / Arduino Uno / Raspberry Pi]

## Sensors/Components
[list here — e.g. smoke sensor, gas sensor, emergency push-button, buzzer/siren, display screen]

---

## FSM States

| State | Description |
|-------|-------------|
| `MONITOR_ENVIRONMENT` | Continuously poll sensors for hazardous conditions |
| `EMERGENCY_DETECTED` | A sensor threshold has been exceeded or emergency button pressed |
| `ALARM_ACTIVE` | Activate audible/visual alarm to alert occupants |
| `EVACUATION_OVERRIDE` | Broadcast override signal — force all gates open, suspend normal FSM operation across all modules |
| `DISPLAY_EVAC` | Show evacuation instructions on display/dashboard and hold until manual reset |

### State Transition Diagram
```
MONITOR_ENVIRONMENT ──[hazard detected]──► EMERGENCY_DETECTED
                                                    │
                                                    ▼
                                             ALARM_ACTIVE
                                                    │
                                                    ▼
                                         EVACUATION_OVERRIDE
                                                    │
                                                    ▼
                                            DISPLAY_EVAC ──[manual reset]──► MONITOR_ENVIRONMENT
```

---

## Interface/Communication
[how this module talks to others — e.g., broadcasts EMERGENCY signal to entrance-gate-module, exit-gate-module, and adaptive-lighting-efficiency over a dedicated interrupt/GPIO line; sends alert to iot-dashboard-hps]

---

## How to Run/Build
[instructions here]
