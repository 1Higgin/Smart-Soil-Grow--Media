#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// -------- WIFI --------
const char* ssid = "YOUR_WIFI";
const char* password = "PASSWORD";

// -------- PINS --------
const int sensorPin = 34;
const int pumpPin   = 25;

// -------- LCD --------
LiquidCrystal_I2C lcd(0x27,16,2);

// -------- WEB SERVER --------
WebServer server(80);

// -------- SOIL --------
int dryValue = 3106;
int wetValue = 1578;
float smoothedMoisture = 0;

// -------- PUMP CONTROL --------
bool watering = false;
bool pumpRunning = false;
unsigned long lastToggle = 0;

const unsigned long pumpOnTime  = 6000;   // 6 sec ON (test)
const unsigned long pumpOffTime = 12000;  // 12 sec OFF (test)

int cycleCount = 0;
const int maxCycles = 3;

// -------- MANUAL CONTROL --------
bool manualOverride = false;
bool manualPumpState = false;

// ----------------------------

int readSoil() {
  long total = 0;
  for(int i=0;i<20;i++){
    total += analogRead(sensorPin);
    delay(3);
  }
  return total / 20;
}

// -------- WEB PAGE --------
void handleRoot() {

  int raw = readSoil();
  int moisture = map(raw, dryValue, wetValue, 0, 100);
  moisture = constrain(moisture,0,100);

  smoothedMoisture = (smoothedMoisture * 0.7) + (moisture * 0.3);
  int stableMoisture = (int)smoothedMoisture;

  String html = "<html><body>";
  html += "<h1>Plant Monitor</h1>";

  html += "<p>Moisture: ";
  html += stableMoisture;
  html += "%</p>";

  html += "<p>Mode: ";
  html += (manualOverride ? "MANUAL" : "AUTO");
  html += "</p>";

  html += "<p>Pump: ";
  html += (manualOverride ? (manualPumpState ? "ON" : "OFF") : (pumpRunning ? "ON" : "OFF"));
  html += "</p>";

  html += "<a href='/on'><button>Manual ON</button></a>";
  html += "<a href='/off'><button>Manual OFF</button></a>";
  html += "<a href='/auto'><button>Auto Mode</button></a>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

// -------- WEB ACTIONS --------
void handleOn() {
  manualOverride = true;
  manualPumpState = true;
  digitalWrite(pumpPin, HIGH);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleOff() {
  manualOverride = true;
  manualPumpState = false;
  digitalWrite(pumpPin, LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleAuto() {
  manualOverride = false;
  server.sendHeader("Location", "/");
  server.send(303);
}

// ----------------------------

void setup() {
  Serial.begin(115200);

  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, LOW);

  lcd.init();
  lcd.backlight();

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());

  // Web routes
  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/auto", handleAuto);

  server.begin();
}

// ----------------------------

void loop() {

  server.handleClient();

  int raw = readSoil();
  int moisture = map(raw, dryValue, wetValue, 0, 100);
  moisture = constrain(moisture,0,100);

  smoothedMoisture = (smoothedMoisture * 0.7) + (moisture * 0.3);
  int stableMoisture = (int)smoothedMoisture;

  unsigned long now = millis();

  // -------- AUTOMATIC MODE --------
  if(!manualOverride){

    if(!watering && stableMoisture < 30){
      watering = true;
      pumpRunning = true;
      cycleCount = 0;
      lastToggle = now;
      digitalWrite(pumpPin, HIGH);
      Serial.println("Auto watering started");
    }

    if(watering && (cycleCount >= maxCycles || stableMoisture > 40)){
      watering = false;
      pumpRunning = false;
      digitalWrite(pumpPin, LOW);
      Serial.println("Auto watering stopped");
    }

    if(watering){
      if(pumpRunning && (now - lastToggle >= pumpOnTime)){
        pumpRunning = false;
        lastToggle = now;
        digitalWrite(pumpPin, LOW);
        cycleCount++;
      }
      else if(!pumpRunning && (now - lastToggle >= pumpOffTime)){
        if(cycleCount < maxCycles && stableMoisture < 40){
          pumpRunning = true;
          lastToggle = now;
          digitalWrite(pumpPin, HIGH);
        }
      }
    }
  }

  // -------- LCD --------
  lcd.setCursor(0,0);
  lcd.print("Moisture:");
  lcd.print(stableMoisture);
  lcd.print("%   ");

  lcd.setCursor(0,1);
  if(manualOverride){
    lcd.print("Manual:");
    lcd.print(manualPumpState ? "ON " : "OFF");
  } else {
    lcd.print("Auto:");
    lcd.print(pumpRunning ? "ON " : "OFF");
  }
  lcd.print("     ");

  delay(500);
}
