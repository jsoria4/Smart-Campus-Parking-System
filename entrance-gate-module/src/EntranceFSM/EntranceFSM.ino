/**
 * @file EntranceFSM.ino
 * @brief Implementation of the entrance FSM on an Arduino R4 with Wifi.
 *
 * Code for the FSM that controls the gate.
 * Uses an RFID reader and an ultrasonic sensor to
 * prompt people entering the parking lot, and then
 * open a gate for them temporarily.
 *
 * @author Landon Wardle
 * @date 5/4/2026
 * @version 1.0
 *
 * @note Ultrasonic sensor in my kit is defective, need a new one to mess with.
 */

/*
 * --------------------------------------------------------------------------------------------------------------------
 * Example to change UID of changeable MIFARE card.
 * --------------------------------------------------------------------------------------------------------------------
 * This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid
 *
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno           Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 */

/* Standard Library Imports */
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <HCSR04.h>

/* State Enums */
enum class State {
  Idle,
  Scan,
  Buzzer,
  OpenGate,
  HoldGateOpen,
  Ultrasense,
  CloseGate,
  IncrementSignal,
  DriveOpenServo,
  DriveCloseServo,
  Emergency,
  EmergencyFlash,
  AtCapacity,
  Count, // sentinel — must stay last; used to size/validate stateHandlers
};

/* State Names in an Array for simplicity */
const char* const stateNames[] = {
  "Idle",
  "Scan",
  "Buzzer",
  "OpenGate",
  "HoldGateOpen",
  "Ultrasense",
  "CloseGate",
  "IncrementSignal",
  "DriveOpenServo",
  "DriveCloseServo",
  "Emergency",
  "EmergencyFlash",
  "AtCapacity",
};

/* LED Colors */
enum class LEDColor {
  Red,
  Yellow,
  Green,
  Blue
};

/* Arduino R4 Settings -----------------------------------------------------------------*/



/* Pins */
const int RST_PIN = 9;   // Configurable, see typical pin layout above
const int SS_PIN = 10;   // Configurable, see typical pin layout above
const int RED_LED = 8;
const int YELLOW_LED = 7;
const int GREEN_LED = 6;
const int SERVO = 3;
const int ULTRA_ECHO = 4;
const int ULTRA_TRIG = 2;
const int BUZZER = 5;
const int CAPACITY_LED = 1;

/* Analog pins for communication */
const int VEHICLE_ENTERED = A0;
const int EMERGENCY_OVERRIDE = A1;
const int PLATE_AUTHORIZED = A2;
const int SPACE_FULL = A3;
const int LOGICAL_HIGH = 650; // 3.3 Volts, threshold for analogRead() comparisons (10-bit ADC scale)
const int LOGICAL_LOW = 0; // 0 Volts

/* analogWrite() on the R4's A0 DAC defaults to 8-bit resolution (0-255). Measured full-scale
 * (255) is ~4.5V on this board rather than the datasheet's 3.3V, so this is scaled down
 * (255 * 3.3/4.5) to hit 3.3V 
 */
const int DAC_HIGH = 175;
const int DAC_LOW = 0;

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

/* Distance threshold (cm) under which the ultrasonic sensor considers something present. */
const float ULTRA_DETECT_CM = 10.0f;

/* Servo value for when the gate is closed. */
const int SERVO_CLOSED = 90;

/* Servo value for when the gate is open. */
const int SERVO_OPEN = 180;

/* Servo d */
const float GATE_TICK = 3000.0f / static_cast<float>(SERVO_OPEN - SERVO_CLOSED);

/* Buzzer Settings -------------------------------------------------------- */

/* Frquency the buzzer emits when turned off (Hz) */
const unsigned int BUZZER_OFF_FREQ = 0;

/* Frquency the buzzer emits when turned on (Hz) */
const unsigned int BUZZER_ON_FREQ = 5000;

/* How long the buzzer sounds for an unverified scan, in milliseconds. */
const unsigned long BUZZER_DURATION_MS = 250UL;

/* Happy Buzzer Settings -----------------------------------------------------------------*/



/**
 * Ascending three-tone chirp played when a card verifies successfully.
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

/* Authorized UID Settings -----------------------------------------------------------------*/



/* Prints every scanned UID to Serial when true. Flip to false before shipping. */
const bool DEBUG_RFID = true;

/**
 * The maximum UID length the reader can produce.
 * MIFARE Classic is 4 bytes; MIFARE DESFire / newer cards are 7;
 * the ISO/IEC 14443-3 spec also allows 10. We size for the max.
 */
const unsigned int MAX_UID_LEN = 10;

/**
 * An authorized UID entry. `length` records how many bytes of `bytes`
 * are meaningful; trailing bytes are ignored during comparison.
 */
struct AuthorizedUID {
  byte length;
  byte bytes[MAX_UID_LEN];
};

/**
 * The list of authorized card UIDs.
 * To add a card: scan it once with debug logging enabled, copy the
 * printed UID here, and reflash.
 */
const AuthorizedUID AUTHORIZED_UIDS[] = {
  // Example 4-byte UID — replace with real values
  {4, {0xD3, 0x1C, 0x80, 0x14}},
  // Example 7-byte UID
  {7, {0x04, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0x00, 0x00, 0x00}}
};

/* Number of entries in AUTHORIZED_UIDS. */
const unsigned int AUTHORIZED_UID_COUNT = sizeof(AUTHORIZED_UIDS) / sizeof(AUTHORIZED_UIDS[0]);
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

/* MFRC522 RFID reader instance. */
MFRC522 mfrc522(SS_PIN, RST_PIN);

/* Servo class. */
Servo servo;

/* Ultrasonic sensor class. */
UltraSonicDistanceSensor distanceSensor(ULTRA_TRIG, ULTRA_ECHO);

/* LEDColor enum value -> pin, indexed by static_cast<int>(LEDColor). */
const int LED_PINS[] = { RED_LED, YELLOW_LED, GREEN_LED, CAPACITY_LED };

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
  setLEDEnabled(LEDColor::Blue, false);
}

/**
 * Enables all LEDs.
 */
void enableLEDS() {
  setLEDEnabled(LEDColor::Red, true);
  setLEDEnabled(LEDColor::Yellow, true);
  setLEDEnabled(LEDColor::Green, true);
  setLEDEnabled(LEDColor::Blue, true);
}

/**
 * Shows the "alert" LED pattern (red only) — waiting for a scan, or rejecting one.
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
 * Checks whether an RFID card has been presented to the reader.
 *
 * @return true if a new card was detected and selected.
 */
bool rfidDetected() {
  if (!mfrc522.PICC_IsNewCardPresent()) return false;
  if (!mfrc522.PICC_ReadCardSerial())   return false;
  return true;
}

/**
 * Prints the most recently read UID to the serial monitor in hex.
 * Useful when enrolling new cards — scan the card with this called
 * and copy the printed bytes into AUTHORIZED_UIDS.
 */
void printLastUID() {
  Serial.print("UID (");
  Serial.print(mfrc522.uid.size);
  Serial.print(" bytes): ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) Serial.print('0');
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1) Serial.print(' ');
  }
  Serial.println();
}

/**
 * Verifies whether the most recently read RFID card is authorized.
 *
 * @note STUB — replace with real UID comparison or backend lookup.
 *
 * @return true if the card is verified.
 */
bool isCardVerified() {
  if (DEBUG_RFID) {
    printLastUID();
  }

  const byte readLen = mfrc522.uid.size;

  for (unsigned int i = 0; i < AUTHORIZED_UID_COUNT; i++) {
    const AuthorizedUID &entry = AUTHORIZED_UIDS[i];

    if (entry.length != readLen) continue;

    if (memcmp(mfrc522.uid.uidByte, entry.bytes, readLen) == 0) {
      Serial.print("Authorized UID matched index ");
      Serial.println(i);
      return true;
    }
  }

  Serial.println("UID not authorized");
  return false;
}

/**
 * Enables or disables the buzzer.
 *
 * @param on true to sound the buzzer, false to silence it.
 */
void setBuzzer(const bool on) {
  tone(BUZZER, on ? BUZZER_ON_FREQ : BUZZER_OFF_FREQ);
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
 * Reads the ultrasonic sensor and reports whether something is in front of the gate.
 *
 * @return true if an object is detected within ULTRA_DETECT_CM.
 */
bool ultrasonicDetected() {
  float reading = distanceSensor.measureDistanceCm();
  Serial.println("Ultra reading:");
  Serial.println(reading);

  return reading <= ULTRA_DETECT_CM;
}

/**
 * @return true if the emergency override input is asserted.
 */
bool isEmergencyOverrideActive() {
  Serial.println(analogRead(EMERGENCY_OVERRIDE));
  return analogRead(EMERGENCY_OVERRIDE) >= LOGICAL_HIGH;
}

/**
 * @return true if the lot has reported itself full.
 */
bool isSpaceFull() {
  return analogRead(SPACE_FULL) >= LOGICAL_HIGH;
}

/**
 * @return true if the current plate reading is authorized.
 */
bool isPlateAuthorized() {
  return analogRead(PLATE_AUTHORIZED) >= LOGICAL_HIGH;
}

/* State Handlers ----------------------------------------------------------------- */



// ── Idle ─────────────────────────────────────────────────────────
// Wait for an RFID card to be presented. Self-loops while no scan.
void handleIdle() {
  showAlertLEDs();

  bool currentPlateAuthorized = isPlateAuthorized();

  if (rfidDetected()) {
    transitionTo(State::Scan);
  } else if (isSpaceFull()) {
    transitionTo(State::AtCapacity);
  } else if (currentPlateAuthorized && currentPlateAuthorized != lastPlateAuthorized) { // On the rising edge
    transitionTo(State::OpenGate);
  }
  lastPlateAuthorized = currentPlateAuthorized;
  // else: self-loop ("no rfid scan")
}

// ── Scan ─────────────────────────────────────────────────────────
// Verify the scanned card. Branch to OpenGate on success or
// Buzzer on failure.
void handleScan() {
  if (isCardVerified()) {
    transitionTo(State::OpenGate);
  } else {
    buzzerStartMs = now;
    setBuzzer(true);
    transitionTo(State::Buzzer);
  }
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
    transitionTo(State::Ultrasense);
  }
  // else: self-loop ("wait 15s")
}

// ── Ultrasense ───────────────────────────────────────────────────
// Keep gate open while something is detected in front of it.
// Once the path is clear, transition to Close.
void handleUltrasense() {
  if (ultrasonicDetected()) {
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

  transitionTo(State::IncrementSignal);
}

// ── IncrementSignal ─────────────────────────────────────────────────
void handleIncrementSignal() {
  analogWrite(VEHICLE_ENTERED, DAC_HIGH);

  if (!elapsedSince(stateEnteredMs, INCREMENT_SIGNAL_DURATION_MS)) {
    return;
  }

  analogWrite(VEHICLE_ENTERED, DAC_LOW);
  transitionTo(State::Idle);
}

// ── At Capacity ─────────────────────────────────────────────────
void handleAtCapacity() {
  setLEDEnabled(LEDColor::Blue, true);

  if (isSpaceFull()) {
    return;
  }

  setLEDEnabled(LEDColor::Blue, false);

  transitionTo(State::Idle);
}

// ── Emergency ─────────────────────────────────────────────────
void handleEmergency() {
  servo.write(SERVO_OPEN);
  disableLEDS();
  setBuzzer(false);
  analogWrite(VEHICLE_ENTERED, DAC_LOW); // force low in case emergency interrupted IncrementSignal mid-pulse

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

/* Dispatch table: index = static_cast<int>(State). Order must match the State enum. */
void (*const stateHandlers[])() = {
  handleIdle,
  handleScan,
  handleBuzzer,
  handleOpenGate,
  handleHoldGateOpen,
  handleUltrasense,
  handleCloseGate,
  handleIncrementSignal,
  handleDriveOpenServo,
  handleDriveCloseServo,
  handleEmergency,
  handleEmergencyFlash,
  handleAtCapacity,
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
  pinMode(CAPACITY_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  pinMode(ULTRA_TRIG, OUTPUT);
  pinMode(ULTRA_ECHO, INPUT);

  servo.attach(SERVO);
  servo.write(SERVO_CLOSED);
  gateSweep.angle = SERVO_CLOSED;

  disableLEDS();
  setBuzzer(false);

  SPI.begin();
  mfrc522.PCD_Init();

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
