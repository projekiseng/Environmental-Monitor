// ============================================================
//  Project: Environmental Monitor with AHT20 + BMP280
//  Author: Ansor Achmad
//  Dibuat : 8 Oktober 2025
// ============================================================

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define BUZZER_PIN 15
#define BTN_UP 27
#define BTN_DOWN 26
#define BTN_LEFT 14
#define BTN_RIGHT 12
#define BTN_SELECT 4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_AHTX0 aht20;
Adafruit_BMP280 bmp;

float tempAHT, humAHT, tempBMP, pressBMP;
float maxTemp = 35.0;
bool alarmActive = false;
unsigned long lastBeep = 0;

void beep(unsigned int duration = 200) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found!");
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  int y = 0;
  int lineH = 9;

  display.setCursor(0, y);
  display.print("Enviromental Monitor");
  y += lineH;
  display.setCursor(0, y);
  display.print("Version 0.1.0");
  y += lineH;
  display.display();
  delay(1000);

  display.setCursor(0, y);
  display.print("Init sensors...");
  y += lineH;
  display.display();
  delay(500);

  // ==== Inisialisasi AHT20 ====
  if (!aht20.begin()) {
    display.setCursor(0, y);
    display.print("AHT20: ERROR!");
    y += lineH;
    display.display();
    while (1);
  } else {
    display.setCursor(0, y);
    display.print("AHT20: OK");
    y += lineH;
    display.display();
  }
  delay(500);

  // ==== Inisialisasi BMP280 ====
  if (!bmp.begin(0x77)) {
    display.setCursor(0, y);
    display.print("BMP280: ERROR!");
    y += lineH;
    display.display();
    while (1);
  } else {
    display.setCursor(0, y);
    display.print("BMP280: OK");
    y += lineH;
    display.display();
  }
  delay(500);

  // ==== Cek Buzzer ====
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
  display.setCursor(0, y);
  display.print("BUZZER: OK");
  display.display();

  delay(1500);
  display.clearDisplay();
  display.display();
}

void readSensors() {
  sensors_event_t hum, temp;
  aht20.getEvent(&hum, &temp);
  tempAHT = temp.temperature;
  humAHT = hum.relative_humidity;
  tempBMP = bmp.readTemperature();
  pressBMP = bmp.readPressure() / 100.0F; // hPa
}

void loop() {
  readSensors();

  // tampilkan data di OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("ENV MONITOR");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setCursor(0, 16);
  display.print("Temp AHT: "); display.print(tempAHT, 1); display.println(" C");
  display.setCursor(0, 26);
  display.print("Humid: "); display.print(humAHT, 1); display.println(" %");
  display.setCursor(0, 36);
  display.print("Temp BMP: "); display.print(tempBMP, 1); display.println(" C");
  display.setCursor(0, 46);
  display.print("Press: "); display.print(pressBMP, 1); display.println(" hPa");
  display.display();

  // cek alarm
  if (tempAHT >= maxTemp && !alarmActive) {
    alarmActive = true;
    lastBeep = millis();
  }

  // buzzer aktif selama alarm
  if (alarmActive) {
    if (millis() - lastBeep > 1000) {
      beep(100);
      lastBeep = millis();
    }
  }

  // tombol SELECT untuk matikan alarm
  if (digitalRead(BTN_SELECT) == LOW && alarmActive) {
    alarmActive = false;
    digitalWrite(BUZZER_PIN, LOW);
  }

  delay(500);
}