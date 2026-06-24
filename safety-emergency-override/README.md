# Safety & Emergency Override

## Module Description
Continuously monitors environmental conditions (smoke, fire, gas, emergency button) and triggers a system-wide evacuation override when a hazard is detected. Overrides all gate and lighting modules, activates alarms, and displays evacuation instructions.

---

## Assigned Member
Connor Wickens

## Language Used
[e.g. SystemVerilog / Python / C++]

## Hardware/Device
[e.g. FPGA DE10-Nano / Arduino Uno / Raspberry Pi]

## Sensors/Components

//IN
flame_sens (fire sensor)
smoke_sens (smoke sensor)
m_reset (Manual reset button)

//OUT
buzzer (Alarm buzzer)
flash_leds (Flashing LEDs)
lcd_evacuate (Send EVACUATE message to LCD display)

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

This module receives no external input signals. It has an active poll that monitors for an emergency, and when a sensor is tripped sends an override signal (force_gates_cw) to the entrance and exit gate modules. While high, this signal will override any changes in either system and force the gates open. This state holds until the override signal is turned off by the manual reset button (m_reset).

## How to Run/Build
[instructions here]
