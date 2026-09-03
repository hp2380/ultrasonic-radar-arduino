import processing.serial.*;

Serial myPort;
String data = "";
float angle, distance;

void setup() {
  size(800, 500);
  smooth();
  myPort = new Serial(this, "COM3", 9600);  // change COM5 to your port
  myPort.bufferUntil('\n');
}

void draw() {
  // black background with slight fade effect
  fill(0, 40);
  noStroke();
  rect(0, 0, width, height);

  // move origin to bottom center
  translate(width / 2, height);

  drawRadar();
  drawSweepLine();
  drawObject();
}

void drawRadar() {
  stroke(0, 255, 0);
  strokeWeight(1);
  noFill();

  // draw arc lines at different distances
  arc(0, 0, 300, 300, PI, TWO_PI);
  arc(0, 0, 220, 220, PI, TWO_PI);
  arc(0, 0, 140, 140, PI, TWO_PI);
  arc(0, 0, 60, 60, PI, TWO_PI);

  // draw angle lines
  line(-300, 0, 300, 0);
  line(0, 0, -300 * cos(radians(30)), -300 * sin(radians(30)));
  line(0, 0, -300 * cos(radians(60)), -300 * sin(radians(60)));
  line(0, 0, -300 * cos(radians(90)), -300 * sin(radians(90)));
  line(0, 0, -300 * cos(radians(120)), -300 * sin(radians(120)));
  line(0, 0, -300 * cos(radians(150)), -300 * sin(radians(150)));
  line(0, 0, 300 * cos(radians(0)), -300 * sin(radians(0)));
}

void drawSweepLine() {
  stroke(30, 250, 60);
  strokeWeight(2);
  line(0, 0,
       150 * cos(radians(angle)),
       -150 * sin(radians(angle)));
}

void drawObject() {
  // only draw blip if object is within 40cm
  if (distance > 0 && distance < 40) {
    fill(255, 10, 10);
    noStroke();
    float x = distance * (150.0 / 40) * cos(radians(angle));
    float y = -distance * (150.0 / 40) * sin(radians(angle));
    ellipse(x, y, 10, 10);
  }
}

void serialEvent(Serial myPort) {
  data = myPort.readStringUntil('\n');
  if (data != null) {
    data = trim(data);
    String[] parts = split(data, ',');
    if (parts.length == 2) {
      angle = float(parts[0]);
      distance = float(parts[1]);
    }
  }
}
