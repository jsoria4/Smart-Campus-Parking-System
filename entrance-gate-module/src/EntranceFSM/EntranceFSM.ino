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
  CounterUpMod,
  DriveOpenServo,
  DriveCloseServo,
  Emergency
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
  "CounterUpMod",
  "DriveOpenServo",
  "DriveCloseServo"
};

/* LED Colors */
enum class LEDColor {
  Red,
  Yellow,
  Green
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

/* Analog pins for communication */
const int VEHICLE_ENTERED = 0;
const int EMERGENCY_OVERRIDE = 1;
const int PLATE_AUTHORIZED = 2;

/* The baud rate to the serial monitor. */
const unsigned int BAUD_RATE = 9600;

/* How long to delay before expecting serial output. */
const unsigned int INIT_DELAY = 100;

/* Gate Settings ------------------------------------------------------------*/



/* How long the gate is held open after opening, in milliseconds. */
const unsigned long GATE_HOLD_MS = 3UL * 1000UL;

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

/* The int value which an Arduino R4 reads as high voltage */
const int HIGH_VOLTAGE = 1023;

/* State  ----------------------------------------------------------------- */



/* General state variables. */
unsigned long now = 0;
unsigned long stateEnteredMs = 0;
unsigned long buzzerStartMs = 0;
unsigned long gateOpenedMs = 0;
unsigned long entryCount = 0;

/* Happy buzzer sequence tracking. */
unsigned int happyBuzzerIndex = 0;
unsigned long happyBuzzerStepMs = 0;
bool happyBuzzerActive = false;

/* Gate sequence tracking. */
unsigned long gateStepMs = 0;
unsigned long gateAngle = 0;

/* Current state of the FSM. */
State currentState;

/* MFRC522 RFID reader instance. */
MFRC522 mfrc522(SS_PIN, RST_PIN);

/* Servo class. */
Servo servo;

/* Ultrasonic sensor class. */
UltraSonicDistanceSensor distanceSensor(ULTRA_TRIG, ULTRA_ECHO);

/* Helper Functions ----------------------------------------------------------------- */

bool isEmergencyOverrideHigh() {
  return analogRead(EMERGENCY_OVERRIDE) == HIGH_VOLTAGE;
}

bool isPlateAuthorizedHigh() {
  return analogRead(PLATE_AUTHORIZED) == HIGH_VOLTAGE;
}

/**
 * Resolves an LEDColor enum to its corresponding pin.
 *
 * @param LED the LED as an enum.
 *
 * @return the pin on the microcontroller that drives the LED.
 */
int getLEDPin(const LEDColor LED) {
  switch (LED) {
    case LEDColor::Red:    return RED_LED;
    case LEDColor::Yellow: return YELLOW_LED;
    case LEDColor::Green:  return GREEN_LED;
    default: return 0;
  }
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
  printLastUID(); // Remove or gate behind a debug flag in production

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
  happyBuzzerIndex = 0;
  happyBuzzerActive = true;
  happyBuzzerStepMs = millis();
  tone(BUZZER, HAPPY_BUZZER_SEQUENCE[0].frequencyHz, HAPPY_BUZZER_SEQUENCE[0].durationMs);
}

/**
 * Non-blocking driver for the happy buzzer chirp.
 * Call every loop iteration; advances to the next tone once the
 * current tone plus its trailing gap have elapsed.
 */
void updateHappyBuzzer() {
  if (!happyBuzzerActive) return;

  unsigned long stepDuration = HAPPY_BUZZER_SEQUENCE[happyBuzzerIndex].durationMs + HAPPY_BUZZER_GAP_MS;
  if (millis() - happyBuzzerStepMs < stepDuration) return;

  happyBuzzerIndex++;
  if (happyBuzzerIndex >= HAPPY_BUZZER_LEN) {
    happyBuzzerActive = false;
    noTone(BUZZER);
    return;
  }

  happyBuzzerStepMs = millis();
  tone(BUZZER,
       HAPPY_BUZZER_SEQUENCE[happyBuzzerIndex].frequencyHz,
       HAPPY_BUZZER_SEQUENCE[happyBuzzerIndex].durationMs);
}

/**
 * Commands the gate servo to the closed position.
 */
void closeGate() {
  for (int i = SERVO_OPEN; i > SERVO_CLOSED; i--) {
      servo.write(i);
      delay(GATE_TICK);
  }

  servo.write(SERVO_CLOSED);
}

/**
 * Reads the ultrasonic sensor and reports whether something is in front of the gate.
 *
 * @return true if an object is detected within ULTRA_DETECT_CM.
 */
bool ultrasonicDetected() {
  float reading = distanceSensor.measureDistanceCm();

  return reading <= ULTRA_DETECT_CM;
}

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

  pinMode(ULTRA_TRIG, OUTPUT);
  pinMode(ULTRA_ECHO, INPUT);

  servo.attach(SERVO);
  servo.write(SERVO_CLOSED);
  gateAngle = SERVO_CLOSED;

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

  switch (currentState) {
    // ── Idle ─────────────────────────────────────────────────────────
    // Wait for an RFID card to be presented. Self-loops while no scan.
    case State::Idle: {
      setLEDEnabled(LEDColor::Red, true);
      setLEDEnabled(LEDColor::Yellow, false);
      setLEDEnabled(LEDColor::Green, false);

      if (rfidDetected()) {
        transitionTo(State::Scan);
      }
      // else: self-loop ("no rfid scan")
      break;
    }

    // ── Scan ─────────────────────────────────────────────────────────
    // Verify the scanned card. Branch to OpenGate on success or
    // Buzzer on failure.
    case State::Scan: {
      if (isCardVerified()) {
        transitionTo(State::OpenGate);
      } else {
        buzzerStartMs = now;
        setBuzzer(true);
        transitionTo(State::Buzzer);
      }
      break;
    }

    // ── Buzzer ───────────────────────────────────────────────────────
    // Sound buzzer for BUZZER_DURATION_MS, then return to Idle.
    case State::Buzzer: {
      setLEDEnabled(LEDColor::Red, true);
      setLEDEnabled(LEDColor::Yellow, false);
      setLEDEnabled(LEDColor::Green, false);

      if (now - buzzerStartMs >= BUZZER_DURATION_MS) {
        setBuzzer(false);
        transitionTo(State::Idle);
      }
      // else: self-loop ("keep buzzing")
      break;
    }

    // ── OpenGate ─────────────────────────────────────────────────────
    // Command the gate to open, then move to HoldGateOpen.
    case State::OpenGate: {
      setLEDEnabled(LEDColor::Red, false);
      setLEDEnabled(LEDColor::Yellow, true);
      setLEDEnabled(LEDColor::Green, false);

      startHappyBuzzer();

      gateAngle = SERVO_CLOSED;
      transitionTo(State::DriveOpenServo);
      break;
    }

    // ── DriveOpenServo ───────────────────────────────────────────────
    // Drives the servo when opening the gate with a for-loop structure
    case State::DriveOpenServo: {
      if (now - gateStepMs < GATE_TICK) return;

      gateStepMs = now;

      if (gateAngle < SERVO_OPEN) {
        gateAngle++;
        servo.write(gateAngle);
        return;
      }

      gateOpenedMs = now;
      transitionTo(State::HoldGateOpen);
      break;
    }

    // ── HoldGateOpen ─────────────────────────────────────────────────
    // Hold the gate open for GATE_HOLD_MS, then check for vehicle
    // presence via ultrasonic.
    case State::HoldGateOpen: {
      setLEDEnabled(LEDColor::Green, true);
      setLEDEnabled(LEDColor::Yellow, false);

      if (now - gateOpenedMs >= GATE_HOLD_MS) {
        transitionTo(State::Ultrasense);
      }
      // else: self-loop ("wait 15s")
      break;
    }

    // ── Ultrasense ───────────────────────────────────────────────────
    // Keep gate open while something is detected in front of it.
    // Once the path is clear, transition to Close.
    case State::Ultrasense: {
      if (ultrasonicDetected()) {
        // self-loop ("dtced")
        break;
      }

      // "!detected" — clear to close
      transitionTo(State::CloseGate);
      break;
    }

    // ── CloseGate ─────────────────────────────────────────────────────
    // Close the gate and move on to update the counter.
    case State::CloseGate: {
      setLEDEnabled(LEDColor::Red, false);
      setLEDEnabled(LEDColor::Yellow, true);
      setLEDEnabled(LEDColor::Green, false);

      gateAngle = SERVO_OPEN;
      transitionTo(State::DriveCloseServo);
      break;
    }

    // ── DriveCloseServo ───────────────────────────────────────────────
    // Drives the servo when closing the gate with a for-loop structure
    case State::DriveCloseServo: {
      if (now - gateStepMs < GATE_TICK) return;

      gateStepMs = now;

      if (gateAngle > SERVO_CLOSED) {
        gateAngle--;
        servo.write(gateAngle);
        return;
      }

      transitionTo(State::CounterUpMod);
      break;
    }

    // ── CounterUpMod ─────────────────────────────────────────────────
    // Increment the entry counter, then return to Idle.
    case State::CounterUpMod: {
      entryCount++;
      Serial.print("Entry count: ");
      Serial.println(entryCount);

      transitionTo(State::Idle);
      break;
    }
    
    // ── Emergency ─────────────────────────────────────────────────
    case State::Emergency: {

      if (isEmergencyHigh()) {
        return;
      }

      transitionTo(State::Idle);
      break;
    }

    // Fail case
    default: {
      Serial.print("ERROR: Unhandled state: ");
      Serial.println(stateNames[static_cast<int>(currentState)]);
      transitionTo(State::Idle);
      break;
    }
  }
}
