# Traffic Congestion Prevention

## Module Description
Monitors the vehicle queue at the parking lot entrance. When a queue is detected (vehicles backing up beyond a threshold), this module signals the entrance gate and external traffic systems to stop admitting vehicles, preventing congestion from spilling onto adjacent roads.

---

## Assigned Member
Robert

## Language Used
Arduino C++

## Hardware/Device
An ESP32 S3 

## Sensors/Components
-Two HC-SR04 ultrasonic sensors


---

## FSM States

| State | Description | Number|
|-------|-------------|-------|
| `IDLE` | No queue detected — normal traffic flow allowed | 0 |
| `QUEUE_DETECTED` | Queue length exceeds threshold — begin congestion response | 1 |
| `STOP` | Halt new vehicle admittance; signal external traffic system to redirect | 2 |

### State Transition Diagram

![The State Transition diagram for the traffic congestion prevention unit](./docs/TrafficCongestionPreventionFSMDiagram.drawio.png)

---

## Interface/Communication
This module sends a DC signal to entrance gate module to tell it to not open when there is a vehicle detected on both sides of the gate

---

## How to Run/Build

### 1. Get a 5V DC power source
### 2. Connect the following S3 dev module pins to the elements below: 

- **Pin 1:** The front ultrasonic sensor trigger pin
- **Pin 2:** The front ultrasonic sensor echo pin
- **Pin 42:** The rear ultrasonic sensor echo pin
- **Pin 41:** A red LED for the stop signal
- **Pin 40:** An LED of any color
    - This LED is the most significant bit representing the state of the congestion prevention FSM
- **Pin 39:** Another LED of any color
    - This LED is the Least significant bit representing the FSM state value
- **Pin 38:** The trigger signal of the rear ultrasonic sensor.

### 3. Connect the sensors and ESP to the 5V power source
### 4. Flash the code to the ESP
