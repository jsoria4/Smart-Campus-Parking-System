/**
  Robert Cromer
  June 2026
  This code contains the program for the FSM that conttolls the traffic congestion prevention unit
  
*/


// Ultrasonic Sensors //
#define FRONT_TRIGGER 1
#define FRONT_ECHO 2
#define REAR_TRIGGER 38
#define REAR_ECHO 42
#define ULTRASONIC_SENSOR_PAUSE_TIME_MILLISECONDS 10
const float MICROSECOND_TO_CENTIMETER_SCALE_FACTOR = 0.0171;


/**
 * LED Output signal
 */
#define CONGESTION_SIGNAL 41

/**
 * A set of LED signals designed to present the state using little-endian computer binary 
 */
#define LED_1 40
#define LED_0 39

/**
 * The maximal distance in centimeters an object must be within hte sensor to be considered a 
 * vehicle
 */
#define MAX_TRIGGER_DISTANCE 15

/**
 * An enum with all of the state names
 */
typedef enum state {
  IDLE,
  QUEUE_DETECTED,
  STOP // Possible remove in future iterations
};

state currentState;

void setup() {
  pinMode(FRONT_TRIGGER, OUTPUT);
  pinMode(FRONT_ECHO, INPUT);
  pinMode(REAR_TRIGGER, OUTPUT);
  pinMode(REAR_ECHO, INPUT);
  pinMode(CONGESTION_SIGNAL, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_0, OUTPUT);

  Serial.begin(9600);
  Serial.println();
}

void loop() {
  float frontSensorDistance = ultrasonicMeasure(FRONT_TRIGGER, FRONT_ECHO);
  delay(100);
  float rearSensorDistance = ultrasonicMeasure(REAR_TRIGGER, REAR_ECHO);

  evaluateOutputs();
  nextState(frontSensorDistance, rearSensorDistance);

  Serial.println("Front distance: " + static_cast<String>(frontSensorDistance));
  Serial.println("Rear distance: " + static_cast<String>(rearSensorDistance));
  Serial.println(currentState);


}

/**
 * Determines what the output signals should be depending on the state. 
 */
void evaluateOutputs() {
  switch (currentState) {
    case IDLE:
      digitalWrite(CONGESTION_SIGNAL, 0);
      break;
    case QUEUE_DETECTED:
      digitalWrite(CONGESTION_SIGNAL, 0);
      break;
    case STOP:
      digitalWrite(CONGESTION_SIGNAL, 1);
      break;
    default:
      digitalWrite(CONGESTION_SIGNAL, 0);
      break;
  }

  digitalWrite(LED_0, currentState % 2);
  digitalWrite(LED_1, currentState / 2 % 2);
}

/**
 * Changes the current state depending on the input signals FRONT_SENSOR and REAR_SENSOR
 */
void nextState(float frontDistance, float rearDistance) {
  switch (currentState) {
    case IDLE:
      if(frontDistance < MAX_TRIGGER_DISTANCE && rearDistance < MAX_TRIGGER_DISTANCE) {
        currentState = QUEUE_DETECTED;
      } else {
        currentState = IDLE;
      }
      break;
    case QUEUE_DETECTED:
      if (rearDistance < MAX_TRIGGER_DISTANCE && frontDistance < MAX_TRIGGER_DISTANCE) {
        currentState = STOP;
      } else { // considers the IDLE state and case where the passageway clears up. 
        currentState = IDLE;
      }
      break;
    case STOP:
      if (rearDistance < MAX_TRIGGER_DISTANCE && frontDistance < MAX_TRIGGER_DISTANCE) {
        currentState = STOP;
      } else {
        currentState = IDLE;
      }
      break;
    default:
      currentState = IDLE;
      break;
  }
}



float ultrasonicMeasure(uint8_t theTriggerPin, uint8_t theEchoPin) { // from arduino get started
  // generate 10-microsecond pulse to TRIG pin
  digitalWrite(theTriggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(theTriggerPin, LOW);

  // measure duration of pulse from ECHO pin
  float duration_us = pulseIn(theEchoPin, HIGH);

  // calculate the distance
  float distance_cm = 0.017 * duration_us;

  return distance_cm;
}
