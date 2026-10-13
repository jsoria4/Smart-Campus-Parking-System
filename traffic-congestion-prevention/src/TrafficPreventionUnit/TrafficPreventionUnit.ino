/**
  Robert Cromer
  June 2026
  This code contains the program for the FSM that conttolls the traffic congestion prevention unit
  
*/


// Beam sensor pins //
#define FRONT_SENSOR 1
#define REAR_SENSOR 2

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
 * An enum with all of the state names
 */
typedef enum state {
  IDLE,
  QUEUE_DETECTED,
  STOP // Possible remove in future iterations
};

state currentState;

void setup() {
  pinMode(FRONT_SENSOR, INPUT);
  pinMode(REAR_SENSOR, INPUT);
  pinMode(CONGESTION_SIGNAL, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_0, OUTPUT);

  Serial.begin(9600);
  Serial.println();
}

void loop() {
  // negating the signals coming from the sensors because the breaker beams will turn off
  // when an item comes between them
  bool frontSensorTripped = !digitalRead(FRONT_SENSOR); 
  delay(100);
  bool rearSensorTripped = !digitalRead(REAR_SENSOR);

  evaluateOutputs();
  nextState(frontSensorTripped, rearSensorTripped);

  Serial.println("Front tripped: " + static_cast<String>(frontSensorTripped));
  Serial.println("Rear distance: " + static_cast<String>(rearSensorTripped));
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
void nextState(bool frontItemDetected, bool rearItemDetected) {
  switch (currentState) {
    case IDLE:
      if(frontItemDetected && rearItemDetected) {
        currentState = QUEUE_DETECTED;
      } else {
        currentState = IDLE;
      }
      break;
    case QUEUE_DETECTED:
      if (frontItemDetected && rearItemDetected) {
        currentState = STOP;
      } else { // considers the IDLE state and case where the passageway clears up. 
        currentState = IDLE;
      }
      break;
    case STOP:
      if (frontItemDetected && rearItemDetected) {
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