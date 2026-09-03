/*
 * radar_logger.ino
 *
 * Data-collection firmware for the radar ML classifier.
 * Sweeps the servo 0-180 degrees, takes a median-filtered HC-SR04 distance
 * reading at each step, and prints one labeled CSV row per reading.
 *
 * Serial protocol (115200 baud, newline-terminated commands):
 *   L<name>   set the class label for subsequent sweeps   e.g.  Lpanel
 *   N<count>  set how many sweeps a GO command records    e.g.  N10
 *   G         record N sweeps and stream them as CSV
 *   H         reprint the CSV header
 *   ?         print current settings
 *
 * CSV columns: sweep_id,label,angle_deg,distance_cm,timestamp_ms
 * distance_cm is -1 when no echo returned within the timeout (no target).
 */

#include <Servo.h>

// ---- pins: same wiring as the original radar sketch ----
const uint8_t SERVO_PIN = 6;   // SG90 signal (orange)
const uint8_t TRIG_PIN  = 9;   // HC-SR04 Trig
const uint8_t ECHO_PIN  = 10;  // HC-SR04 Echo

// ---- sweep geometry ----
const uint8_t  ANGLE_START   = 0;
const uint8_t  ANGLE_END     = 180;
const uint8_t  ANGLE_STEP    = 2;     // 91 points per sweep
const uint16_t SETTLE_MS     = 30;    // servo travel time per step
const uint8_t  PINGS_PER_PT  = 3;     // median filter width (must be odd)
const uint16_t PING_GAP_MS   = 10;    // let echoes die between pings
const uint32_t ECHO_TIMEOUT  = 25000UL; // us -> about 430 cm max range

Servo servo;

char     label[16]   = "unlabeled";
uint16_t sweepCount  = 5;    // sweeps recorded per G command
uint16_t sweepId     = 0;    // increments for the life of the session

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  servo.attach(SERVO_PIN);
  servo.write(ANGLE_START);
  delay(500);

  printHeader();
  Serial.println(F("# ready. commands: L<label> N<count> G H ?"));
}

void loop() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.length() == 0) return;

  char op = cmd.charAt(0);
  String arg = cmd.substring(1);
  arg.trim();

  switch (op) {
    case 'L':
    case 'l':
      arg.toCharArray(label, sizeof(label));
      Serial.print(F("# label = "));
      Serial.println(label);
      break;

    case 'N':
    case 'n': {
      int n = arg.toInt();
      if (n > 0) {
        sweepCount = n;
        Serial.print(F("# sweeps per run = "));
        Serial.println(sweepCount);
      } else {
        Serial.println(F("# bad count"));
      }
      break;
    }

    case 'G':
    case 'g':
      for (uint16_t i = 0; i < sweepCount; i++) {
        recordSweep();
      }
      Serial.println(F("# done"));
      break;

    case 'H':
    case 'h':
      printHeader();
      break;

    case '?':
      printStatus();
      break;

    default:
      Serial.println(F("# unknown command"));
      break;
  }
}

void printHeader() {
  Serial.println(F("sweep_id,label,angle_deg,distance_cm,timestamp_ms"));
}

void printStatus() {
  Serial.print(F("# label="));      Serial.print(label);
  Serial.print(F(" sweeps="));      Serial.print(sweepCount);
  Serial.print(F(" next_id="));     Serial.println(sweepId);
}

/*
 * Sweeps out and back, printing a CSV row at every step of the outbound
 * pass only. Sweeping back without logging returns the servo to the start
 * position so every sweep is recorded in the same direction -- mixing
 * directions would put a systematic servo-lag offset into the features.
 */
void recordSweep() {
  for (uint8_t a = ANGLE_START; a <= ANGLE_END; a += ANGLE_STEP) {
    servo.write(a);
    delay(SETTLE_MS);

    long d = medianDistance();

    Serial.print(sweepId);      Serial.print(',');
    Serial.print(label);        Serial.print(',');
    Serial.print(a);            Serial.print(',');
    Serial.print(d);            Serial.print(',');
    Serial.println(millis());
  }

  servo.write(ANGLE_START);
  delay(400);
  sweepId++;
}

/*
 * Takes PINGS_PER_PT readings and returns the median. The median rejects a
 * single wild reading (a stray echo, or a miss) without smearing the edges
 * of an object the way an average would -- edges are exactly the signal the
 * classifier needs.
 */
long medianDistance() {
  long samples[PINGS_PER_PT];

  for (uint8_t i = 0; i < PINGS_PER_PT; i++) {
    samples[i] = pingOnce();
    delay(PING_GAP_MS);
  }

  // insertion sort -- PINGS_PER_PT is tiny, so this is plenty
  for (uint8_t i = 1; i < PINGS_PER_PT; i++) {
    long key = samples[i];
    int8_t j = i - 1;
    while (j >= 0 && samples[j] > key) {
      samples[j + 1] = samples[j];
      j--;
    }
    samples[j + 1] = key;
  }

  return samples[PINGS_PER_PT / 2];
}

/*
 * One HC-SR04 measurement. Returns distance in cm, or -1 on timeout.
 * -1 means "nothing returned an echo", which is real information: it is
 * what empty space looks like, and it is different from "very far away".
 */
long pingOnce() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long dur = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT);
  if (dur == 0) return -1;

  // speed of sound ~343 m/s -> 29.1 us per cm, halved for the round trip
  return (long)(dur / 58);
}
