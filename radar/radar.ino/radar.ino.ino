#include <Servo.h>  // import servo library so we can control the servo motor

Servo radarServo;  // create a servo object — this represents the physical servo

// define which Arduino pins everything is connected to
const int trigPin  = 9;   // trig pin sends the ultrasonic pulse out
const int echoPin  = 10;  // echo pin listens for the pulse to come back
const int servoPin = 6;   // servo signal wire

// sweep limits — kept off the servo's hard stops so the sensor cable
// isn't flexed at its extremes every single pass
const int SWEEP_MIN = 15;
const int SWEEP_MAX = 165;

// pulseIn timeout in microseconds. 25000us of round-trip travel is about
// 4.3m of range. Without this, a missing echo blocks for a FULL SECOND.
const unsigned long ECHO_TIMEOUT = 25000UL;

// === SENSOR FUNCTIONS ===

// sends one ultrasonic pulse and returns the distance in cm,
// or -1 if no echo came back before the timeout
long getDistance() {
  digitalWrite(trigPin, LOW);       // make sure trig starts LOW (clean state)
  delayMicroseconds(2);             // wait 2 microseconds

  digitalWrite(trigPin, HIGH);      // send a HIGH pulse to trigger the sensor
  delayMicroseconds(10);            // hold HIGH for 10 microseconds — minimum needed to trigger HC-SR04
  digitalWrite(trigPin, LOW);       // pull trig back LOW

  // measure how long echo pin stays HIGH (in microseconds)
  // this is the time sound took to travel to the object and back
  long duration = pulseIn(echoPin, HIGH, ECHO_TIMEOUT);

  // pulseIn returns 0 on timeout. That is NOT a distance of zero —
  // it means nothing was detected. Report it as -1 so it can't be
  // mistaken for an object sitting against the sensor face.
  if (duration == 0) return -1;

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

  // return middle value — throws out the 2 highest and 2 lowest spikes
  // any -1 values sort to the bottom, so a median of -1 means at least
  // 3 of 5 reads found nothing — a genuine "nothing there"
  return readings[2];
}

// === SETUP ===

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  radarServo.attach(servoPin);
  radarServo.write(90);     // start at center
  delay(500);               // let it settle before anything else
  Serial.begin(9600);
}

// === MAIN LOOP ===

void loop() {
  // sweep forward
  for (int angle = SWEEP_MIN; angle <= SWEEP_MAX; angle++) {
    radarServo.write(angle);              // move servo to this angle
    delay(50);                            // wait 50ms for servo to physically reach the angle
    long distance = getStableDistance();  // take a stable distance reading at this angle

    Serial.print(angle);      // send angle over serial
    Serial.print(",");        // comma separator
    Serial.println(distance); // send distance, newline tells the receiver data ended
  }

  // sweep back
  for (int angle = SWEEP_MAX; angle >= SWEEP_MIN; angle--) {
    radarServo.write(angle);
    delay(50);
    long distance = getStableDistance();

    Serial.print(angle);
    Serial.print(",");
    Serial.println(distance);
  }
}