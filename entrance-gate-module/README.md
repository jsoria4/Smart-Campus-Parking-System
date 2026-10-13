# Entrance Gate Module

## Module Description
Controls the physical entry gate of the parking lot. A vehicle is let in when the Automatic License Plate Reader signals that it read an authorized plate. (RFID card scanning is still in the code but disabled, since the license plate reader replaces the card reader.) The gate servo then sweeps open, holds for 3 seconds, stays open while the IR break-beam sensor detects the vehicle, sweeps closed, and pulses the Space Management Counter to count the vehicle. The module also holds the gate shut while the lot is full or congested, sounds the buzzer for rejected plates, and forces the gate open with flashing lights and an alarm while the emergency override is active.

---

## Assigned Member
Landon

## Language Used
C++

## Hardware/Device
Arduino Uno R4 WiFi

## Sensors/Components
- MFRC522 RFID Reader (disabled, replaced by the license plate reader)
- Small Servo Motor
- IR Break-Beam Sensor
- LED Indicators (red, yellow, green, and blue for at capacity)
- Passive Buzzer

---

## FSM States

| State | Description |
|-------|-------------|
| `Idle` | Red LED on, gate signal low. Waits for the lot to fill, congestion, an authorized plate (rising edge), or a buzzer signal, checked in that order. RFID scans are read but ignored |
| `Scan` | Compare the scanned card's UID against the authorized UID list. Currently unreachable since RFID scanning is disabled |
| `Buzzer` | Card or plate rejected. Red LED on and the buzzer sounds for 250 ms |
| `OpenGate` | Access granted. Yellow LED on, play the happy chirp, raise the gate open signal, and start the opening sweep |
| `DriveOpenServo` | Sweep the servo one degree every 50 ms until it reaches the open angle |
| `HoldGateOpen` | Green LED on. Hold the gate open for 3 seconds |
| `BreakSense` | Keep the gate open while the break-beam sensor detects the vehicle |
| `CloseGate` | Path is clear. Yellow LED on and start the closing sweep |
| `DriveCloseServo` | Sweep the servo back to the closed angle, then drop the gate open signal |
| `IncrementSignal` | Pulse the vehicle entered signal at 3.3 V for 100 ms so the counter increments |
| `AtCapacity` | Blue LED on. Wait until the lot is no longer full |
| `Congested` | Wait until the congestion signal clears |
| `Emergency` | Snap the gate open, turn the LEDs and buzzer off, and raise the gate open signal. Returns to `Idle` (closing the gate) once the override is released |
| `EmergencyFlash` | All LEDs and the buzzer on for 250 ms, alternating with `Emergency` |

### State Transition Diagram
```
Reset ──► Idle ─┬─[space full]─────────────► AtCapacity ──[!space full]──► Idle
                ├─[congested]──────────────► Congested ───[!congested]───► Idle
                ├─[buzzer signal]──────────► Buzzer ──────[250 ms]───────► Idle
                ├─[rfid (disabled)]────────► Scan ─┬─[unverified]────────► Buzzer
                │                                  │
                │                             [verified]
                │                                  │
                └─[plate authorized edge]──► OpenGate ◄┘
                                                 │
                                                 ▼
                                          DriveOpenServo ──[gate open]──► HoldGateOpen ──[3 s]──► BreakSense
                                                                                                       │
                                                                                                 [beam clear]
                                                                                                       ▼
                                                                                                   CloseGate
                                                                                                       │
                                                                                                       ▼
                                          Idle ◄──[100 ms]── IncrementSignal ◄──[gate closed]── DriveCloseServo
```

Emergency override, checked every loop from any state:
```
Any state ──[emergency override high]──► Emergency ──[250 ms]──► EmergencyFlash
                                           │   ▲                        │
                                           │   └────────[250 ms]────────┘
                               [emergency override low]
                                           │
                                           ▼
                                         Idle
```

---

## Interface/Communication
- (output) vehicle_entered (A0) -> A brief 3.3 V pulse (100 ms) sent to FSM #1 Space Management & Controller to increment the vehicle count
- (output) gate_open (D4) -> High while the gate is open or opening, tells the Automatic License Plate Reader when the gate cycle has finished
- (input) emergency_override (A1) -> A signal from FSM #3 Safety & Emergency Override to force the gate open for emergency purposes
- (input) plate_authorized (A2) -> A signal from the Automatic License Plate Reader (Raspberry Pi 5) confirming an authorized plate was read, acted on at its rising edge
- (input) space_full (A3) -> A signal from FSM #1 indicating there are no more parking spots available
- (input) congested (A4) -> A signal from FSM #6 Traffic Congestion Prevention indicating traffic is backed up and the gate should stay closed
- (input) trigger_buzzer (A5) -> A signal from the Automatic License Plate Reader indicating a plate was rejected, sounds the buzzer

All analog inputs count as high at or above 650 on the 10-bit ADC, just under 3.3 V.

---

## How to Run/Build
1. Open EntranceFSM.ino in the src/EntranceFSM folder in Arduino IDE.
2. Install the "Arduino UNO R4 Boards" package from the Boards Manager, and the MFRC522 library (by GithubCommunity) from the Library Manager
3. Connect an Arduino Uno R4 WiFi to your device via USB and select it as the board
4. Press the arrow in the top left to deploy the code to the Microcontroller
5. Done!

If RFID scanning is re-enabled, to authorize a new card open the Serial Monitor at 9600 baud with `DEBUG_RFID` set to `true`, scan the card, copy the printed UID into `AUTHORIZED_UIDS`, and redeploy.
