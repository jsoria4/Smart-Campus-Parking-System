# Adaptive Lighting Efficiency

## Module Description
Manages the parking lot's lighting system by measuring ambient light levels and switching between day, night, and smart dimming modes. Reduces energy consumption by adjusting light intensity based on real-time conditions and occupancy data.

---

## Assigned Member
[NAME]

## Language Used
[e.g. SystemVerilog / Python / C++]

## Hardware/Device
[e.g. FPGA DE10-Nano / Arduino Uno / Raspberry Pi]

## Sensors/Components
[list here — e.g. photoresistor/LDR, PWM-controlled LED drivers, RTC module]

---

## FSM States

| State | Description |
|-------|-------------|
| `MEASURE_LIGHT` | Sample the ambient light sensor and compare against thresholds |
| `DAY_MODE` | Ambient light is sufficient — turn off or minimize artificial lighting |
| `NIGHT_MODE` | Ambient light is below threshold — activate full lighting |
| `SMART_DIMMING` | Lot is empty or partially occupied at night — dim lights to save energy |

### State Transition Diagram
```
                    ┌──────────────────────────────────────────┐
                    │                                          │
                    ▼                                          │
             MEASURE_LIGHT                                     │
               │       │                                       │
    [light high]│       │[light low]                           │
               ▼        ▼                                      │
           DAY_MODE   NIGHT_MODE ──[low occupancy]──► SMART_DIMMING
               │          │                                    │
               └──────────┴────────────────────────────────────┘
                        [re-measure]
```

---

## Interface/Communication
[how this module talks to others — e.g., receives occupancy level from space-management-counter to decide between NIGHT_MODE and SMART_DIMMING; receives EVACUATION_OVERRIDE from safety-emergency-override to switch to full brightness]

---

## How to Run/Build
[instructions here]
