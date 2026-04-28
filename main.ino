#define BLYNK_TEMPLATE_ID "YOUR BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Door Lock Project"
#define BLYNK_AUTH_TOKEN "YOUR BLYNK_AUTH_TOKEN"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

char ssid[] = "WIFI_NAME";
char pass[] = "WIFI_PASSWORD";

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define TRIG_PIN 5
#define ECHO_PIN 18
#define RELAY_PIN 4
#define BUTTON_PIN 14
#define LDR_PIN 34

#define DARK_THRESHOLD 1500

// -------- STATES --------
bool lightState = false;
bool studyMode = false;
bool sleepMode = false;

int occupancyCount = 0;
bool lastButtonState = HIGH;

// timing
unsigned long lastTriggerTime = 0;
unsigned long studyModeChangedTime = 0;
const int cooldown = 2000;

// button
unsigned long buttonPressTime = 0;
bool buttonHeld = false;
const int LONG_PRESS_TIME = 2000;

// distance history
long history[5] = {0};
int histIndex = 0;
bool bufferFilled = false;

// animation flags
String lastEvent = "";

// -------- DISTANCE --------
long getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;

  long d = duration * 0.034 / 2;
  if (d < 2 || d > 400) return -1;

  return d;
}

// -------- FILTER --------
long getFilteredDistance() {
  long readings[5];
  int count = 0;

  for (int i = 0; i < 5; i++) {
    long d = getDistance();
    if (d != -1) readings[count++] = d;
    delay(5);
  }

  if (count == 0) return -1;

  for (int i = 0; i < count - 1; i++) {
    for (int j = i + 1; j < count; j++) {
      if (readings[i] > readings[j]) {
        long t = readings[i];
        readings[i] = readings[j];
        readings[j] = t;
      }
    }
  }

  return readings[count / 2];
}

// -------- TREND --------
void updateHistory(long d) {
  history[histIndex] = d;
  histIndex = (histIndex + 1) % 5;
  if (histIndex == 0) bufferFilled = true;
}

bool isIncreasing() {
  for (int i = 0; i < 4; i++)
    if (history[i] >= history[i + 1]) return false;
  return true;
}

bool isDecreasing() {
  for (int i = 0; i < 4; i++)
    if (history[i] <= history[i + 1]) return false;
  return true;
}

// -------- BLYNK --------
BLYNK_WRITE(V0) {
  studyMode = param.asInt();
  if (studyMode && !sleepMode) lightState = true;
}

BLYNK_WRITE(V2) {
  sleepMode = param.asInt();

  if (sleepMode) {
    lightState = false;
  } else {
    int ldr = analogRead(LDR_PIN);
    if (ldr < DARK_THRESHOLD) lightState = true;
  }
}

// -------- LCD --------
void showMainScreen(long d) {
  lcd.setCursor(0, 0);
  lcd.print("P:");
  lcd.print(occupancyCount);
  lcd.print(" D:");
  lcd.print(d);
  lcd.print("cm   ");

  lcd.setCursor(0, 1);
  lcd.print("L:");
  lcd.print(lightState ? "ON " : "OFF");

  lcd.print(" ");

  if (sleepMode) lcd.print("SLP");
  else if (studyMode) lcd.print("STD");
  else lcd.print("NRM");
}

// -------- ANIMATION --------
void showAnimation(String type) {
  lcd.clear();

  if (type == "ENTER") {
    lcd.setCursor(0, 0);
    lcd.print(">> ENTER >>");
  } else if (type == "EXIT") {
    lcd.setCursor(0, 0);
    lcd.print("<< EXIT <<");
  }

  delay(500);
  lcd.clear();
}

// -------- SETUP --------
void setup() {
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LDR_PIN, INPUT);

  lcd.init();
  lcd.backlight();

  lcd.print("Starting...");
  delay(1000);
  lcd.clear();
}

// -------- LOOP --------
void loop() {

  Blynk.run();

  // BUTTON
  bool currentButton = digitalRead(BUTTON_PIN);

  if (currentButton == LOW && lastButtonState == HIGH) {
    buttonPressTime = millis();
    buttonHeld = true;
  }

  if (currentButton == HIGH && lastButtonState == LOW) {
    if (buttonHeld) {
      unsigned long pressDuration = millis() - buttonPressTime;

      if (pressDuration >= LONG_PRESS_TIME) {
        studyMode = !studyMode;
        if (studyMode && !sleepMode) lightState = true;
      } else {
        lightState = !lightState;
      }
      buttonHeld = false;
    }
  }

  lastButtonState = currentButton;

  long d = -1;

  if (!sleepMode) {
    d = getFilteredDistance();

    if (d != -1) {
      updateHistory(d);

      if (bufferFilled && millis() - lastTriggerTime > cooldown) {

        if (isIncreasing()) {
          occupancyCount++;
          if (occupancyCount < 0) occupancyCount = 0;

          showAnimation("ENTER");
          Serial.println("================================");
          Serial.println("🚶 ENTER DETECTED");
          Serial.print("People Count: ");
          Serial.println(occupancyCount);

          int ldr = analogRead(LDR_PIN);
          if (ldr < DARK_THRESHOLD) lightState = true;

          lastTriggerTime = millis();
        }

        else if (isDecreasing()) {
          occupancyCount--;
          if (occupancyCount < 0) occupancyCount = 0;

          showAnimation("EXIT");
          Serial.println("================================");
          Serial.println("🚶 EXIT DETECTED");
          Serial.print("People Count: ");
          Serial.println(occupancyCount);

          if (!studyMode && occupancyCount == 0) {
            lightState = false;
          }

          lastTriggerTime = millis();
        }
      }
    }
  }

  // RELAY
  digitalWrite(RELAY_PIN, lightState ? HIGH : LOW);

  // LCD UPDATE
  static unsigned long lastLCD = 0;
  if (millis() - lastLCD > 300) {
    showMainScreen(d);
    lastLCD = millis();
  }
}
