# IoT Dashboard & HPS

## Module Description
Central data hub running on the Hard Processor System (HPS) or a Raspberry Pi / PC. Aggregates real-time data from all other modules, logs historical occupancy and event data, serves the operator dashboard, and supports remote configuration adjustments.

---

## Assigned Member
[NAME]

## Language Used
[e.g. Python / C / Node.js]

## Hardware/Device
[e.g. Raspberry Pi 4 / DE10-Nano HPS / PC]

## Sensors/Components
[list here — e.g. Ethernet/Wi-Fi interface, touchscreen display, local SQLite/CSV storage]

---

## FSM States

| State | Description |
|-------|-------------|
| `SYNC_DATA` | Poll or receive data from all connected modules (space count, gate events, sensor readings) |
| `LOG_HISTORY` | Persist incoming event data to local storage (CSV / SQLite database) |
| `DASHBOARD_UPDATE` | Refresh the operator-facing display with latest occupancy, alerts, and statistics |
| `REMOTE_ADJUST` | Process an incoming remote command (e.g., change capacity limit, silence alarm, force gate) |

### State Transition Diagram
```
        ┌────────────────────────────────────────────┐
        │                                            │
        ▼                                            │
   SYNC_DATA ──► LOG_HISTORY ──► DASHBOARD_UPDATE ──┘
        │
        │[remote command received]
        ▼
  REMOTE_ADJUST ──► SYNC_DATA
```

---

## Interface/Communication
[how this module talks to others — e.g., receives UART/SPI serial data from FPGA and Arduino modules; exposes a local REST API or MQTT broker for remote dashboard access; sends REMOTE_ADJUST commands back to modules via serial]

---

## How to Run/Build
[instructions here]
