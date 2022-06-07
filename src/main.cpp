#include <Arduino.h>
#include <AccelStepper.h>

#define echoPin 4 
#define trigPin 5 
#define pirPin A0
#define endSwitchPin 2
#define dirPin 6
#define stepPin 7
#define motorInterfaceType 1

// Machine settings
int handUpDelay = 2000;
int handReachedPosition = 200;
int handUpPosition = 680;
int slowSpeed = 100;
int fastSpeed = 2500;
int slowAccel = 200;
int fastAccel = 1000;
int handReachTresholdCm = 25;
int handReachTimer = 10000; //10 seconds

int prevTime = 0;

// Defines variables for ultrasonic sensor
long duration; // variable for the duration of sound wave travel
int distance; // variable for the distance measurement

// Create a new instance of the AccelStepper class:
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);

// States of the state machine
enum MachineStates {INIT_MACHINE, MACHINE_READY, HAND_REACHED, HAND_UP};
MachineStates state;

void endSwitchTouched(){
  //Detach interrupt to prevent new interrupt from occuring, set zero position then move to next state
  detachInterrupt(digitalPinToInterrupt(endSwitchPin));
  stepper.stop();
  stepper.setCurrentPosition(0);
  state = MACHINE_READY;
}

void setup() {
  //Set the initial state
  state = INIT_MACHINE;  

  // Setup for ultra sonic sensor
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an OUTPUT
  pinMode(echoPin, INPUT); // Sets the echoPin as an INPUT
  
  // Setup for endswitch (interruped attached)
  pinMode(endSwitchPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(endSwitchPin), endSwitchTouched, LOW);

  // Setup for PIR sensor
  pinMode(pirPin, INPUT);
  Serial.begin(9600);
}

void loop() {
  //State machine
  switch(state){
    case INIT_MACHINE: //Move arm to start position (end switch), when end switch touched (interrupt) go to next state
      stepper.setMaxSpeed(slowSpeed);
      stepper.setAcceleration(slowAccel);
      stepper.moveTo(-2000);
      stepper.runToPosition();
    break;
    case MACHINE_READY: //Check PIR sensor, if triggered: move hand to hand_reach postion, then go to next state
      if(digitalRead(pirPin)){
        stepper.moveTo(handReachedPosition);
        stepper.runToPosition();
        state = HAND_REACHED;
        prevTime = millis();
        Serial.print("MACHINE_READY :prev time= ");
        Serial.println(prevTime);
      }
    break;
    case HAND_REACHED: // Check ultra sonic sensor, if distance below treshold move hand up then go to next state
      // Ultra sonic sensor code block
      digitalWrite(trigPin, LOW);
      delayMicroseconds(2);
      // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);
      // Reads the echoPin, returns the sound wave travel time in microseconds
      duration = pulseIn(echoPin, HIGH);
      // Calculating the distance
      distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)
      // Displays the distance on the Serial Monitor

      if(millis()-prevTime>handReachTimer){
        Serial.print("HAND_REACHED :millis()= ");
        Serial.println(millis());
        Serial.print("HAND_REACHED :prev time= ");
        Serial.println(prevTime);
        
        attachInterrupt(digitalPinToInterrupt(endSwitchPin), endSwitchTouched, LOW);
        state = INIT_MACHINE;
        break;
      }
      
      if(distance <= handReachTresholdCm){
        stepper.setMaxSpeed(fastSpeed);
        stepper.setAcceleration(fastAccel);
        stepper.moveTo(handUpPosition);
        stepper.runToPosition();
        state = HAND_UP;
      }
    break;
    case HAND_UP: // Wait for a few seconds then go to first state again
      delay(handUpDelay);
      attachInterrupt(digitalPinToInterrupt(endSwitchPin), endSwitchTouched, LOW);
      state = INIT_MACHINE;
    break;
  }
}
