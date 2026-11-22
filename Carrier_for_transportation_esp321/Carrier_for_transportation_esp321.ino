#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= LINE SENSORS =================
const int linePins[5] = {34, 35, 36, 39, 26};
const int LINE_THRESHOLD = 2000;

// ================= ULTRASONIC =================
const int TRIG = 5;
const int ECHO = 18;
const int OBSTACLE_LIMIT = 20;  // cm

// ================= PACKAGE SENSOR =================
const int PACKAGE_SENSOR = 27;

// ================= DOCK SWITCHES =================
const int DOCK1 = 32;
const int DOCK2 = 33;

// ================= EMERGENCY STOP =================
const int ESTOP = 25;

// ================= FRONT MOTOR DRIVER =================
const int FL_IN1 = 13;
const int FL_IN2 = 14;
const int FR_IN1 = 16;
const int FR_IN2 = 17;

// ================= REAR MOTOR DRIVER =================
const int RL_IN1 = 19;
const int RL_IN2 = 23;
const int RR_IN1 = 4;
const int RR_IN2 = 15;

// ================= SPEED SETTINGS =================
int baseSpeed = 130;
int turnSpeed = 90;

// ================= SYSTEM STATE =================
bool packageDetected = false;
int distanceCM = -1;

// =================================================
// SETUP
// =================================================

void setup() {
  Serial.begin(115200);

  // Line sensors
  for (int i = 0; i < 5; i++) {
    pinMode(linePins[i], INPUT);
  }

  // Ultrasonic
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // Package sensor
  pinMode(PACKAGE_SENSOR, INPUT);

  // Dock switches
  pinMode(DOCK1, INPUT_PULLUP);
  pinMode(DOCK2, INPUT_PULLUP);

  // Emergency stop
  pinMode(ESTOP, INPUT_PULLUP);

  // Motor pins
  pinMode(FL_IN1, OUTPUT);
  pinMode(FL_IN2, OUTPUT);
  pinMode(FR_IN1, OUTPUT);
  pinMode(FR_IN2, OUTPUT);
  pinMode(RL_IN1, OUTPUT);
  pinMode(RL_IN2, OUTPUT);
  pinMode(RR_IN1, OUTPUT);
  pinMode(RR_IN2, OUTPUT);

  // LCD
  lcd.init();
  lcd.backlight();

  stopMotors();

  lcd.setCursor(0, 0);
  lcd.print("Carrier Starting");
  delay(1000);

  lcd.clear();
  lcd.print("System Ready");

  Serial.println("Carrier Ready");
}

// =================================================
// MAIN LOOP
// =================================================

void loop() {

  // 1. Emergency stop
  if (digitalRead(ESTOP) == LOW) {
    stopMotors();
    displayStatus("EMERGENCY STOP");
    return;
  }

  // 2. Dock detection
  if (digitalRead(DOCK1) == LOW ||
      digitalRead(DOCK2) == LOW) {
    stopMotors();
    displayStatus("DOCKED");
    return;
  }

  // 3. Check obstacle
  distanceCM = getDistance();

  if (distanceCM > 0 && distanceCM <= OBSTACLE_LIMIT) {
    stopMotors();
    displayStatus("OBSTACLE");
    return;
  }

  // 4. Check package
  packageDetected = digitalRead(PACKAGE_SENSOR);

  // 5. Follow line
  followLine();

  // 6. Serial output
  Serial.print("Distance: ");
  Serial.print(distanceCM);
  Serial.print(" cm | Package: ");
  Serial.println(packageDetected ? "Detected" : "Absent");

  delay(20);
}

// =================================================
// LINE FOLLOWING
// =================================================

void followLine() {

  int values[5];

  for (int i = 0; i < 5; i++) {
    values[i] = analogRead(linePins[i]);
  }

  bool s1 = values[0] > LINE_THRESHOLD;
  bool s2 = values[1] > LINE_THRESHOLD;
  bool s3 = values[2] > LINE_THRESHOLD;
  bool s4 = values[3] > LINE_THRESHOLD;
  bool s5 = values[4] > LINE_THRESHOLD;

  if (s3) {
    moveForward(baseSpeed, baseSpeed);
  }
  else if (s2) {
    moveForward(turnSpeed, baseSpeed);
  }
  else if (s1) {
    moveForward(0, baseSpeed);
  }
  else if (s4) {
    moveForward(baseSpeed, turnSpeed);
  }
  else if (s5) {
    moveForward(baseSpeed, 0);
  }
  else {
    stopMotors();
  }
}

// =================================================
// MOTOR CONTROL
// =================================================

void motorControl(int pin1, int pin2, int speedValue) {

  speedValue = constrain(speedValue, -255, 255);

  if (speedValue > 0) {
    analogWrite(pin1, speedValue);
    analogWrite(pin2, 0);
  }
  else if (speedValue < 0) {
    analogWrite(pin1, 0);
    analogWrite(pin2, abs(speedValue));
  }
  else {
    analogWrite(pin1, 0);
    analogWrite(pin2, 0);
  }
}

void moveForward(int leftSpeed, int rightSpeed) {

  motorControl(FL_IN1, FL_IN2, leftSpeed);
  motorControl(RL_IN1, RL_IN2, leftSpeed);

  motorControl(FR_IN1, FR_IN2, rightSpeed);
  motorControl(RR_IN1, RR_IN2, rightSpeed);
}

void stopMotors() {

  motorControl(FL_IN1, FL_IN2, 0);
  motorControl(FR_IN1, FR_IN2, 0);
  motorControl(RL_IN1, RL_IN2, 0);
  motorControl(RR_IN1, RR_IN2, 0);
}

// =================================================
// ULTRASONIC SENSOR
// =================================================

int getDistance() {

  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 30000);

  if (duration == 0) {
    return -1;
  }

  return (int)(duration * 0.0343 / 2);
}

// =================================================
// LCD STATUS
// =================================================

void displayStatus(const char* message) {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);

  Serial.println(message);
}