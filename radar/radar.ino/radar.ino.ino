#include <Servo.h>  // import servo library so we can control the servo motor

Servo radarServo;  // create a servo object — this represents the physical servo

// define which Arduino pins the sensor is connected to
const int trigPin = 9;   // trig pin sends the ultrasonic pulse out
const int echoPin = 10;  // echo pin listens for the pulse to come back

// === SENSOR FUNCTIONS ===

// sends one ultrasonic pulse and returns the raw distance in cm
long getDistance() {
  digitalWrite(trigPin, LOW);       // make sure trig starts LOW (clean state)
  delayMicroseconds(2);             // wait 2 microseconds

  digitalWrite(trigPin, HIGH);      // send a HIGH pulse to trigger the sensor
  delayMicroseconds(10);            // hold HIGH for 10 microseconds — minimum needed to trigger HC-SR04
  digitalWrite(trigPin, LOW);       // pull trig back LOW

  long duration = pulseIn(echoPin, HIGH);  // measure how long echo pin stays HIGH (in microseconds)
                                           // this is the time sound took to travel to object and back

  return duration * 0.034 / 2;     // convert time to cm
                                   // 0.034 = speed of sound in cm per microsecond
                                   // divide by 2 because sound travels TO object AND back
}

// takes 5 readings and returns the median (middle value)
// this removes random spikes and gives a stable distance
long getStableDistance() {
  long readings[5];  // array to store 5 readings

  // take 5 readings with 10ms gap between each
  for (int i = 0; i < 5; i++) {
    readings[i] = getDistance();
    delay(10);
  }

  // bubble sort the 5 readings from smallest to largest
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4 - i; j++) {
      if (readings[j] > readings[j+1]) {
        long t = readings[j];          // swap readings[j] and readings[j+1]
        readings[j] = readings[j+1];
        readings[j+1] = t;
      }
    }
  }

  return readings[2];  // return middle value — throws out 2 highest and 2 lowest spikes
}

// === SETUP ===

void setup() {
  pinMode(trigPin, OUTPUT);   // trig sends signals OUT to sensor
  pinMode(echoPin, INPUT);    // echo receives signals IN from sensor
  radarServo.attach(6);       // servo signal wire is on pin 6
  Serial.begin(9600);         // open serial communication at 9600 baud
}

// === MAIN LOOP ===

void loop() {
  // sweep from 0° to 180°
  for (int angle = 0; angle <= 180; angle++) {
    radarServo.write(angle);              // move servo to this angle
    delay(50);                            // wait 50ms for servo to physically reach the angle
    long distance = getStableDistance();  // take a stable distance reading at this angle

    Serial.print(angle);      // send angle over serial to Processing
    Serial.print(",");        // comma separator
    Serial.println(distance); // send distance, newline tells Processing data ended
  }

  // sweep back from 180° to 0°
  for (int angle = 180; angle >= 0; angle--) {
    radarServo.write(angle);
    delay(50);
    long distance = getStableDistance();

    Serial.print(angle);
    Serial.print(",");
    Serial.println(distance);
  }
}