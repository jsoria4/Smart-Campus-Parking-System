# Automatic License Plate Reader

## Module Description
Reads the license plate of a vehicle waiting at the entrance gate. When the IR break-beam sensor is broken, the Raspberry Pi captures a frame from the camera, finds the plate with a YOLO model (exported to NCNN), and reads its text with EasyOCR. The plate is then checked against the authorized vehicle list: a valid plate signals the Entrance Gate Module to open the gate, and an invalid plate sounds the buzzer for a 3 second cooldown. The FSM is event-driven, GPIO edge interrupts are queued and the main loop sleeps until one arrives, so no CPU is spent polling the pins while idle.

---

## Assigned Members
Landon & Connor

## Language Used
Python

## Hardware/Device
Raspberry Pi 5

## Sensors/Components
- Raspberry Pi Camera Module
- IR Break-Beam Sensor
- Buzzer

---

## FSM States

| State | Description |
|-------|-------------|
| `INIT` | Load the YOLO model, start the camera, and initialize the EasyOCR reader |
| `IDLE` | Waiting for a vehicle to break the beam. Left whenever the beam is broken, including on entry from `INIT`, `BUZZER`, or `OPEN_GATE` if a vehicle is still present |
| `TAKE_PICTURE` | Capture a frame, detect the plate with YOLO, and read its text with EasyOCR |
| `VERIFY_PLATE` | Check the plate text against the authorized vehicle list |
| `OPEN_GATE` | Plate is valid. Hold the open gate signal high until the gate reports closed |
| `BUZZER` | Plate is invalid. Hold the buzzer signal high for a 3 second cooldown |

### State Transition Diagram
```
Reset ──► INIT ──► IDLE ◄──────────────────────────────────────────┐
                    │                                              │
             [beam broken]                                         │
                    │                                              │
                    ▼                                              │
              TAKE_PICTURE ──[picture processed]──► VERIFY_PLATE   │
                                                         │         │
                              ┌──────────────────────────┴────┐    │
                         [valid plate]                [invalid plate]
                              │                               │    │
                              ▼                               ▼    │
                          OPEN_GATE                         BUZZER │
                              │                               │    │
                        [gate closed]                [cooldown over]
                              │                               │    │
                              └───────────────┬───────────────┘    │
                                              └────────────────────┘
```
Events with no arrow out of the current state are ignored (e.g. the beam breaking again while a picture is processing). Each time `IDLE` is entered the beam is checked again, so a vehicle that is still in the beam after the buzzer or after the gate closes is scanned again.

---

## Interface/Communication
- (output) open_gate (GPIO 22) -> High only while in `OPEN_GATE`, tells FSM #7 Entrance Gate Module that a plate was read and authorized
- (output) buzzer (GPIO 10) -> High only while in `BUZZER`, signals the driver that their plate was rejected
- (input) breaker_sensor (GPIO 17) -> IR break-beam sensor, active high, indicates a vehicle is waiting at the gate
- (input) gate_closed (GPIO 27) -> A signal from FSM #7 Entrance Gate Module, active high, indicating the gate has closed behind the vehicle

---

## How to Run/Build
1. On the Raspberry Pi, create a virtual environment that can see the system packages (picamera2, gpiozero): `python3 -m venv --system-site-packages ALPRvenv`
2. Activate it from the `auto-license-plate-reader` folder: `source ALPRvenv/bin/activate`
3. Install the dependencies: `pip install easyocr ultralytics opencv-python`, then `pip install --force-reinstall --no-cache-dir simplejpeg`
4. Plug in the camera (without it you will get an index out of range error), and wire the sensor and signal pins listed above
5. From the `src` folder, run `python alpr.py`
6. Done!

> **Warning: don't skip the `simplejpeg` reinstall in step 3.** picamera2 comes from the system packages, and its `simplejpeg` dependency is a compiled binary built against the system's older NumPy. Installing ultralytics and easyocr pulls a newer NumPy into the virtual environment, which takes priority over the system copy. The system `simplejpeg` binary is then out of date for the NumPy it loads, so importing picamera2 fails with a NumPy binary incompatibility error (typically `numpy.dtype size changed`) and the program won't start. Force-reinstalling `simplejpeg` inside the virtual environment installs a build that matches the newer NumPy. Run it again any time a `pip install` changes the NumPy version.

To check the camera and model on their own, run `python pipeline_test.py` from `src`, which takes one picture and prints the plate it read. The FSM unit tests don't need any hardware: `python -m unittest test_alpr_fsm -v` from `src`.
