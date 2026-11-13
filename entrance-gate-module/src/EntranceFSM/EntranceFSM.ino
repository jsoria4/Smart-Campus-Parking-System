/**
 * @file EntranceFSM.ino
 * @brief Implementation of the entrance FSM on an Arduino Nano.
 *
 * Code for the FSM that controls the gate.
 * Opens the gate when the license plate reader signals an
 * authorized plate, and uses a break-beam sensor to hold it
 * open until the vehicle has passed.
 *
 * @author Landon Wardle
 * @date 5/4/2026
 * @version 1.0
 */

/* Standard Library Imports */
#include <Servo.h>

/* State Enums */
enum class State {
  Idle,
  Buzzer,
  OpenGate,
  HoldGateOpen,
  BreakSense,
  CloseGate,
  IncrementSignal,
  DriveOpenServo,
  DriveCloseServo,
  Emergency,
  EmergencyFlash,
  AtCapacity,
  Congested,
  Count, // sentinel — must stay last; used to size/validate stateHandlers
};

/* State Names in an Array for simplicity */
const char* const stateNames[] = {
  "Idle",
  "Buzzer",
  "OpenGate",
  "HoldGateOpen",
  "BreakSense",
  "CloseGate",
  "IncrementSignal",
  "DriveOpenServo",
  "DriveCloseServo",
  "Emergency",
  "EmergencyFlash",
  "AtCapacity",
  "Congested"
};

/* LED Colors */
enum class LEDColor {
  Red,
  Yellow,
  Green
};

/* Arduino Nano Settings -----------------------------------------------------------------*/



/* Pins */
const int RED_LED = 8;
const int YELLOW_LED = 7;
const int GREEN_LED = 6;
const int SERVO = 3;
const int BREAK_BEAM_READ = 2;
const int BUZZER = 5;
// TODO: GATE_OPEN drives 5V when HIGH, but it feeds a Raspberry Pi GPIO input
// (ALPR gate_closed, GPIO 27) that is only 3.3V tolerant. Before wiring into the
// real system, drop it to 3.3V with the same 1k/2k divider used on VEHICLE_ENTERED
// (or a level shifter) so it doesn't damage the Pi.
const int GATE_OPEN = 4;

/* Analog pins for communication */
const int VEHICLE_ENTERED = A0;
const int EMERGENCY_OVERRIDE = A1;
const int PLATE_AUTHORIZED = A2;
const int SPACE_FULL = A3;
const int CONGESTED = A4;
const int TRIGGER_BUZZER = A5;
const int LOGICAL_HIGH = 400; // ~2 Volts, threshold for analogRead() comparisons (10-bit ADC scale, 5V ref); leaves margin below the Pi's 3.3V high
const int LOGICAL_LOW = 0; // 0 Volts

/* The Nano has no DAC, so VEHICLE_ENTERED is driven as a plain 5V digital output.
 * It must pass through a resistor divider (A0 -> 1k -> Pi GPIO -> 2k -> GND, ~3.33V)
 * before reaching the 3.3V-tolerant Pi input.
 */

/* The baud rate to the serial monitor. */
const unsigned int BAUD_RATE = 9600;

/* How long to delay before expecting serial output. */
const unsigned int INIT_DELAY = 100;

/* Gate Settings ------------------------------------------------------------*/



/* How long the gate is held open after opening, in milliseconds. */
const unsigned long GATE_HOLD_MS = 3UL * 1000UL;

/* How long the increment signal is held high for. */
const unsigned long INCREMENT_SIGNAL_DURATION_MS = 100UL;

/* How long the emergency flash lights are held on for. */
const unsigned long EMERGENCY_FLASH_DURATION_MS = 250UL;

/* Servo value for when the gate is closed. */
const int SERVO_CLOSED = 130;

/* Servo value for when the gate is open. */
const int SERVO_OPEN = 70;

/* Servo d */
const float GATE_TICK = 3000.0f / abs(static_cast<float>(SERVO_OPEN - SERVO_CLOSED));

/* Buzzer Settings -------------------------------------------------------- */

/* Frquency the buzzer emits when turned on (Hz) */
const unsigned int BUZZER_ON_FREQ = 5000;

/* How long the buzzer sounds when the buzzer signal is triggered, in milliseconds. */
const unsigned long BUZZER_DURATION_MS = 250UL;

/* Happy Buzzer Settings -----------------------------------------------------------------*/



/**
 * Ascending three-tone chirp played when the gate opens.
 * Each entry is {frequency_hz, duration_ms}. A short gap is inserted
 * between tones so they don't blur together.
 */
struct BuzzerTone {
  unsigned int frequencyHz;
  unsigned int durationMs;
};

const BuzzerTone HAPPY_BUZZER_SEQUENCE[] = {
  { 880,  90 },   // A5
  { 1175, 90 },   // D6
  { 1568, 140 }   // G6
};

/* Number of tones in the happy buzzer sequence. */
const unsigned int HAPPY_BUZZER_LEN = sizeof(HAPPY_BUZZER_SEQUENCE) / sizeof(HAPPY_BUZZER_SEQUENCE[0]);

/* Silence inserted between consecutive tones in the sequence, in milliseconds. */
const unsigned int HAPPY_BUZZER_GAP_MS = 30;

/* State  ----------------------------------------------------------------- */



/* General state variables. */
unsigned long now = 0;
unsigned long stateEnteredMs = 0;
unsigned long buzzerStartMs = 0;
unsigned long gateOpenedMs = 0;
bool lastPlateAuthorized = false;

/* Happy buzzer sequence tracking. `index`, `stepMs`, and `active` are meaningless
 * apart from each other — they only make sense together as "where the chirp is right now." */
struct HappyBuzzer {
  unsigned int index;
  unsigned long stepMs;
  bool active;
};
HappyBuzzer happyBuzzer = { 0, 0, false };

/* Gate sequence tracking. `lastStepMs` and `angle` are meaningless apart from
 * each other — they only make sense together as "where the sweep is right now." */
struct ServoSweep {
  unsigned long lastStepMs;
  int angle;
};
ServoSweep gateSweep = { 0, 0 };

/* Current state of the FSM. */
State currentState;

/* Servo class. */
Servo servo;

/* LEDColor enum value -> pin, indexed by static_cast<int>(LEDColor). */
const int LED_PINS[] = { RED_LED, YELLOW_LED, GREEN_LED };

/**
 * Resolves an LEDColor enum to its corresponding pin.
 *
 * @param LED the LED as an enum.
 *
 * @return the pin on the microcontroller that drives the LED.
 */
int getLEDPin(const LEDColor LED) {
  return LED_PINS[static_cast<int>(LED)];
}

/**
 * Sets an LED to be enabled.
 *
 * @param LED the LED as an enum.
 * @param enabled if the LED is enabled.
 */
void setLEDEnabled(const LEDColor LED, const bool enabled) {
  digitalWrite(getLEDPin(LED), enabled ? HIGH : LOW);
}

/**
 * Disables all LEDs.
 */
void disableLEDS() {
  setLEDEnabled(LEDColor::Red, false);
  setLEDEnabled(LEDColor::Yellow, false);
  setLEDEnabled(LEDColor::Green, false);
}

/**
 * Enables all LEDs.
 */
void enableLEDS() {
  setLEDEnabled(LEDColor::Red, true);
  setLEDEnabled(LEDColor::Yellow, true);
  setLEDEnabled(LEDColor::Green, true);
}

/**
 * Shows the "alert" LED pattern (red only) — gate closed, waiting for a vehicle, or buzzing.
 */
void showAlertLEDs() {
  setLEDEnabled(LEDColor::Red, true);
  setLEDEnabled(LEDColor::Yellow, false);
  setLEDEnabled(LEDColor::Green, false);
}

/**
 * Shows the "transit" LED pattern (yellow only) — the gate is mid-swing.
 */
void showTransitLEDs() {
  setLEDEnabled(LEDColor::Red, false);
  setLEDEnabled(LEDColor::Yellow, true);
  setLEDEnabled(LEDColor::Green, false);
}

/**
 * Transitions the FSM to a new state and records when it was entered.
 *
 * @param next the state to transition to.
 */
void transitionTo(const State next) {
  currentState = next;
  stateEnteredMs = millis();
  Serial.print("Transition -> ");
  Serial.println(stateNames[static_cast<int>(next)]);
}

/**
 * @param since a millis() timestamp to measure from.
 * @param duration how long must have passed, in milliseconds.
 * @return true once `duration` has passed since `since`.
 */
bool elapsedSince(const unsigned long since, const unsigned long duration) {
  return now - since >= duration;
}

/**
 * Enables or disables the buzzer.
 *
 * @param on true to sound the buzzer, false to silence it.
 */
void setBuzzer(const bool on) {
  if (on) {
    tone(BUZZER, BUZZER_ON_FREQ);
  } else {
    noTone(BUZZER); // tone(pin, 0) divides by zero on AVR and emits a garbage waveform
  }
}

/**
 * Starts the happy buzzer chirp from the beginning.
 * Plays the first tone immediately; subsequent tones are advanced
 * by repeated calls to updateHappyBuzzer().
 */
void startHappyBuzzer() {
  happyBuzzer.index = 0;
  happyBuzzer.active = true;
  happyBuzzer.stepMs = millis();
  tone(BUZZER, HAPPY_BUZZER_SEQUENCE[0].frequencyHz, HAPPY_BUZZER_SEQUENCE[0].durationMs);
}

/**
 * Non-blocking driver for the happy buzzer chirp.
 * Call every loop iteration; advances to the next tone once the
 * current tone plus its trailing gap have elapsed.
 */
void updateHappyBuzzer() {
  if (!happyBuzzer.active) return;

  unsigned long stepDuration = HAPPY_BUZZER_SEQUENCE[happyBuzzer.index].durationMs + HAPPY_BUZZER_GAP_MS;
  if (millis() - happyBuzzer.stepMs < stepDuration) return;

  happyBuzzer.index++;
  if (happyBuzzer.index >= HAPPY_BUZZER_LEN) {
    happyBuzzer.active = false;
    noTone(BUZZER);
    return;
  }

  happyBuzzer.stepMs = millis();
  tone(BUZZER,
       HAPPY_BUZZER_SEQUENCE[happyBuzzer.index].frequencyHz,
       HAPPY_BUZZER_SEQUENCE[happyBuzzer.index].durationMs);
}

/**
 * Advances the gate servo one tick toward `target`, respecting GATE_TICK timing.
 * Intended to be called every loop iteration from a Drive*Servo state.
 *
 * @param target the angle to drive toward (SERVO_OPEN or SERVO_CLOSED).
 * @return true once the servo has reached target; false if still moving.
 */
bool stepServoToward(const int target) {
  if (now - gateSweep.lastStepMs < GATE_TICK) return false;

  gateSweep.lastStepMs = now;

  if (gateSweep.angle == target) return true;

  gateSweep.angle += (gateSweep.angle < target) ? 1 : -1;
  servo.write(gateSweep.angle);
  return false;
}

/**
 * Reads the Break Beam Sensor
 *
 * @return true if an object is detected, false otherwise.
 */
bool breakBeamDetected() {
  return digitalRead(BREAK_BEAM_READ) == LOW;
}

/**
 * @return true if the emergency override input is asserted.
 */
bool isEmergencyOverrideActive() {
  return analogRead(EMERGENCY_OVERRIDE) >= LOGICAL_HIGH;
}

/**
 * @return true if the lot has reported itself full.
 */
bool isSpaceFull() {
  return analogRead(SPACE_FULL) >= LOGICAL_HIGH;
}

/**
 * @return true if the lot is congested.
 */
bool isCongested() {
  return analogRead(CONGESTED) >= LOGICAL_HIGH;
}

/**
 * @return true if buzzer signal is high.
 */
bool isBuzzerSignalHigh() {
  return analogRead(TRIGGER_BUZZER) >= LOGICAL_HIGH;
}

/**
 * @return true if the current plate reading is authorized.
 */
bool isPlateAuthorized() {
  return analogRead(PLATE_AUTHORIZED) >= LOGICAL_HIGH;
}

/* State Handlers ----------------------------------------------------------------- */



// ── Idle ─────────────────────────────────────────────────────────
// Wait for an authorized plate or another input signal. Self-loops while none.
void handleIdle() {
  showAlertLEDs();

  bool currentPlateAuthorized = isPlateAuthorized();

  digitalWrite(GATE_OPEN, LOW);

  if (isSpaceFull()) {
    transitionTo(State::AtCapacity);
  } else if (isCongested()) {
    transitionTo(State::Congested);
  } else if (currentPlateAuthorized && currentPlateAuthorized != lastPlateAuthorized) { // On the rising edge
    transitionTo(State::OpenGate);
  } else if (isBuzzerSignalHigh()) {
    buzzerStartMs = now;
    setBuzzer(true);
    transitionTo(State::Buzzer);
  }
  lastPlateAuthorized = currentPlateAuthorized;
  // else: self-loop ("no input")
}

// ── Buzzer ───────────────────────────────────────────────────────
// Sound buzzer for BUZZER_DURATION_MS, then return to Idle.
void handleBuzzer() {
  showAlertLEDs();

  if (elapsedSince(buzzerStartMs, BUZZER_DURATION_MS)) {
    setBuzzer(false);
    transitionTo(State::Idle);
  }
  // else: self-loop ("keep buzzing")
}

// ── OpenGate ─────────────────────────────────────────────────────
// Command the gate to open, then move to HoldGateOpen.
void handleOpenGate() {
  showTransitLEDs();

  startHappyBuzzer();

  digitalWrite(GATE_OPEN, HIGH);

  gateSweep.angle = SERVO_CLOSED;
  transitionTo(State::DriveOpenServo);
}

// ── DriveOpenServo ───────────────────────────────────────────────
// Drives the servo when opening the gate.
void handleDriveOpenServo() {
  if (!stepServoToward(SERVO_OPEN)) return;

  gateOpenedMs = now;
  transitionTo(State::HoldGateOpen);
}

// ── HoldGateOpen ─────────────────────────────────────────────────
// Hold the gate open for GATE_HOLD_MS, then check for vehicle
// presence via ultrasonic.
void handleHoldGateOpen() {
  setLEDEnabled(LEDColor::Green, true);
  setLEDEnabled(LEDColor::Yellow, false);

  if (elapsedSince(gateOpenedMs, GATE_HOLD_MS)) {
    transitionTo(State::BreakSense);
  }
  // else: self-loop ("wait 15s")
}

// ── BreakSense ───────────────────────────────────────────────────
// Keep gate open while something is detected in front of it.
// Once the path is clear, transition to Close.
void handleBreakSense() {
  if (breakBeamDetected()) {
    return; // self-loop ("dtced")
  }

  // "!detected" — clear to close
  transitionTo(State::CloseGate);
}

// ── CloseGate ─────────────────────────────────────────────────────
// Close the gate and move on to update the counter.
void handleCloseGate() {
  showTransitLEDs();

  gateSweep.angle = SERVO_OPEN;
  transitionTo(State::DriveCloseServo);
}

// ── DriveCloseServo ───────────────────────────────────────────────
// Drives the servo when closing the gate.
void handleDriveCloseServo() {
  if (!stepServoToward(SERVO_CLOSED)) return;

  digitalWrite(GATE_OPEN, LOW);
  transitionTo(State::IncrementSignal);
}

// ── IncrementSignal ─────────────────────────────────────────────────
void handleIncrementSignal() {
  digitalWrite(VEHICLE_ENTERED, HIGH);

  if (!elapsedSince(stateEnteredMs, INCREMENT_SIGNAL_DURATION_MS)) {
    return;
  }

  digitalWrite(VEHICLE_ENTERED, LOW);
  transitionTo(State::Idle);
}

// ── At Capacity ─────────────────────────────────────────────────
void handleAtCapacity() {
  if (isSpaceFull()) {
    return;
  }

  transitionTo(State::Idle);
}

// ── Emergency ─────────────────────────────────────────────────
void handleEmergency() {
  servo.write(SERVO_OPEN);
  disableLEDS();
  setBuzzer(false);
  digitalWrite(VEHICLE_ENTERED, LOW); // force low in case emergency interrupted IncrementSignal mid-pulse
  digitalWrite(GATE_OPEN, HIGH);

  if (!isEmergencyOverrideActive()) {
    servo.write(SERVO_CLOSED);
    transitionTo(State::Idle);
    return;
  }

  if (!elapsedSince(stateEnteredMs, EMERGENCY_FLASH_DURATION_MS)) {
    return;
  }

  transitionTo(State::EmergencyFlash);
}

// ── Emergency Flash ─────────────────────────────────────────────────
void handleEmergencyFlash() {
  enableLEDS();
  setBuzzer(true);

  if (!elapsedSince(stateEnteredMs, EMERGENCY_FLASH_DURATION_MS)) {
    return;
  }

  transitionTo(State::Emergency);
}

// ── Congested ─────────────────────────────────────────────────
void congested() {
  //TODO Screen state here!

  if (isCongested()) {
    return;
  }

  transitionTo(State::Idle);
}

/* Dispatch table: index = static_cast<int>(State). Order must match the State enum. */
void (*const stateHandlers[])() = {
  handleIdle,
  handleBuzzer,
  handleOpenGate,
  handleHoldGateOpen,
  handleBreakSense,
  handleCloseGate,
  handleIncrementSignal,
  handleDriveOpenServo,
  handleDriveCloseServo,
  handleEmergency,
  handleEmergencyFlash,
  handleAtCapacity,
  congested
};

static_assert(sizeof(stateHandlers) / sizeof(stateHandlers[0]) == static_cast<int>(State::Count),
              "stateHandlers is out of sync with the State enum — add/remove a handler to match");

/* Setup & Loop ----------------------------------------------------------------- */



/**
 * Called when the microcontroller initializes. Manages state initialization.
 */
void setup() {
  Serial.begin(BAUD_RATE);

  delay(INIT_DELAY);

  Serial.println("Initializing...");

  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(GATE_OPEN, OUTPUT);
  pinMode(VEHICLE_ENTERED, OUTPUT);

  pinMode(BREAK_BEAM_READ, INPUT);

  servo.attach(SERVO);
  servo.write(SERVO_CLOSED);
  gateSweep.angle = SERVO_CLOSED;

  disableLEDS();
  setBuzzer(false);

  Serial.println("Initialization complete!");

  transitionTo(State::Idle);
}

/**
 * loop function that drives the FSM.
 */
void loop() {
  now = millis();

  updateHappyBuzzer();

  if (isEmergencyOverrideActive() && currentState != State::Emergency && currentState != State::EmergencyFlash) {
    transitionTo(State::Emergency);
  }

  stateHandlers[static_cast<int>(currentState)]();
}
