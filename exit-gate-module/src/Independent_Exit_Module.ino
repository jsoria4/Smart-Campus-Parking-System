/**
 * @file ExitFSM.ino
 * @brief Implementation of the exit FSM on an Arduino.
 *
 * Code for the FSM that controls the exit gate.
 * Uses an ultrasonic sensor to detect a departing vehicle,
 * opens the gate barrier, waits for the vehicle to clear,
 * closes the gate, then fires a one-shot exit pulse to FSM #1
 * (Space Management & Counter) on the FPGA.
 *
 * @author Jerry
 * @date 6/27/2026
 * @version 1.0
 */

/* Standard Library Imports */
#include "SR04.h"
#include <Servo.h>

/* State Enums */
enum class State {
  Idle,
  AllowExit,
  OpenGate,
  DriveOpenServo,
  HoldGate,
  CloseGate,
  DriveCloseServo,
  ExitPulse
};

/* State Names in an Array for simplicity */
const char* const stateNames[] = {
  "Idle",
  "AllowExit",
  "OpenGate",
  "DriveOpenServo",
  "HoldGate",
  "CloseGate",
  "DriveCloseServo",
  "ExitPulse"
};

/* Arduino Settings -----------------------------------------------------------------*/



/* Pins */
const int TRIG_PIN       = 12;
const int ECHO_PIN       = 11;
const int GREEN_LED      = LED_BUILTIN;
const int SERVO_PIN      = 9;
const int EXIT_PULSE_PIN = 7;   // Goes to FPGA FSM #1 GPIO input

/* The baud rate to the serial monitor. */
const unsigned int BAUD_RATE = 9600;

/* Gate Settings ------------------------------------------------------------*/



/* Distance threshold (cm) under which the ultrasonic sensor considers something present. */
const float ULTRA_DETECT_CM = 20.0f;

/* How long the gate is held open after opening, in milliseconds. */
const unsigned long GATE_HOLD_MS = 3UL * 1000UL;

/* Servo value for when the gate is closed. */
const int SERVO_CLOSED = 0;

/* Servo value for when the gate is open. */
const int SERVO_OPEN = 90;

/*
 * Milliseconds per degree of servo travel.
 * Total sweep time is 3000 ms over (SERVO_OPEN - SERVO_CLOSED) degrees.
 */
const float GATE_TICK = 3000.0f / static_cast<float>(SERVO_OPEN - SERVO_CLOSED);

/* Exit Pulse Settings -------------------------------------------------------- */

/* Width of the exit pulse sent to the FPGA counter, in milliseconds. */
const unsigned long EXIT_PULSE_MS = 100UL;

/* State  ----------------------------------------------------------------- */



/* General state variables. */
unsigned long now           = 0;
unsigned long stateEnteredMs = 0;
unsigned long gateOpenedMs  = 0;
unsigned long pulseStartMs  = 0;

/* Gate sequence tracking. */
unsigned long gateStepMs = 0;
int           gateAngle  = 0;

/* Current state of the FSM. */
State currentState;

/* Ultrasonic sensor instance. */
SR04 sr04 = SR04(ECHO_PIN, TRIG_PIN);

/* Servo instance. */
Servo servo;

/* Helper Functions ----------------------------------------------------------------- */



/**
 * Transitions the FSM to a new state and records when it was entered.
 *
 * @param next the state to transition to.
 */
void transitionTo(const State next) {
  currentState  = next;
  stateEnteredMs = millis();
  Serial.print("Transition -> ");
  Serial.println(stateNames[static_cast<int>(next)]);
}

/**
 * Reads the ultrasonic sensor and reports whether a vehicle is present.
 *
 * @return true if an object is detected within ULTRA_DETECT_CM.
 */
bool ultrasonicDetected() {
  long reading = sr04.Distance();
  Serial.print(reading);
  Serial.println("cm");
  return reading <= static_cast<long>(ULTRA_DETECT_CM);
}

/* Setup & Loop ----------------------------------------------------------------- */



/**
 * Called when the microcontroller initializes. Manages state initialization.
 */
void setup() {
  Serial.begin(BAUD_RATE);

  Serial.println("Initializing...");

  pinMode(GREEN_LED,      OUTPUT);
  pinMode(EXIT_PULSE_PIN, OUTPUT);

  digitalWrite(GREEN_LED,      LOW);
  digitalWrite(EXIT_PULSE_PIN, LOW);   // Idle low

  servo.attach(SERVO_PIN);
  servo.write(SERVO_CLOSED);
  gateAngle = SERVO_CLOSED;

  Serial.println("Initialization complete!");

  transitionTo(State::Idle);
}

/**
 * Loop function that drives the FSM.
 */
void loop() {
  now = millis();

  switch (currentState) {

    // ── Idle ─────────────────────────────────────────────────────────
    // Monitor front_sensor_2. Transition as soon as a vehicle is
    // detected within range.
    case State::Idle: {
      digitalWrite(GREEN_LED, LOW);

      if (ultrasonicDetected()) {
        transitionTo(State::AllowExit);
      }
      // else: self-loop ("no vehicle")
      break;
    }

    // ── AllowExit ────────────────────────────────────────────────────
    // Vehicle confirmed at the exit sensor. Light the green LED and
    // proceed to open the gate.
    case State::AllowExit: {
      digitalWrite(GREEN_LED, HIGH);
      Serial.println("Vehicle detected — allowing exit.");
      transitionTo(State::OpenGate);
      break;
    }

    // ── OpenGate ─────────────────────────────────────────────────────
    // Arm the servo drive variables, then hand off to DriveOpenServo.
    case State::OpenGate: {
      gateAngle   = SERVO_CLOSED;
      gateStepMs  = now;
      transitionTo(State::DriveOpenServo);
      break;
    }

    // ── DriveOpenServo ───────────────────────────────────────────────
    // Sweep the servo open one degree per GATE_TICK ms (non-blocking).
    case State::DriveOpenServo: {
      if (now - gateStepMs < static_cast<unsigned long>(GATE_TICK)) break;

      gateStepMs = now;

      if (gateAngle < SERVO_OPEN) {
        gateAngle++;
        servo.write(gateAngle);
        break;
      }

      gateOpenedMs = now;
      transitionTo(State::HoldGate);
      break;
    }

    // ── HoldGate ─────────────────────────────────────────────────────
    // Keep the gate open for GATE_HOLD_MS, then wait for the vehicle
    // to clear before closing.
    case State::HoldGate: {
      if (now - gateOpenedMs < GATE_HOLD_MS) break;  // self-loop ("wait 3s")

      if (ultrasonicDetected()) break;  // self-loop ("vehicle still present")

      transitionTo(State::CloseGate);
      break;
    }

    // ── CloseGate ────────────────────────────────────────────────────
    // Arm the servo drive variables for the closing sweep.
    case State::CloseGate: {
      digitalWrite(GREEN_LED, LOW);
      gateAngle  = SERVO_OPEN;
      gateStepMs = now;
      transitionTo(State::DriveCloseServo);
      break;
    }

    // ── DriveCloseServo ──────────────────────────────────────────────
    // Sweep the servo closed one degree per GATE_TICK ms (non-blocking).
    case State::DriveCloseServo: {
      if (now - gateStepMs < static_cast<unsigned long>(GATE_TICK)) break;

      gateStepMs = now;

      if (gateAngle > SERVO_CLOSED) {
        gateAngle--;
        servo.write(gateAngle);
        break;
      }

      // Gate fully closed — vehicle has cleared.
      pulseStartMs = now;
      digitalWrite(EXIT_PULSE_PIN, HIGH);
      transitionTo(State::ExitPulse);
      break;
    }

    // ── ExitPulse ────────────────────────────────────────────────────
    // Hold the exit pulse HIGH for EXIT_PULSE_MS, then release and
    // return to Idle. One pulse = one car exited → FSM #1 decrements.
    case State::ExitPulse: {
      if (now - pulseStartMs < EXIT_PULSE_MS) break;  // self-loop ("pulsing")

      digitalWrite(EXIT_PULSE_PIN, LOW);
      Serial.println("Exit pulse fired -> FSM #1 counter increment.");
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
