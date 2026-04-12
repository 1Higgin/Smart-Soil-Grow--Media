#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =====================
// OLED CONFIG
// =====================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// =====================
// WIFI
// =====================
const char* ssid = "HUAWEI-B535-A157";
const char* password = "7G14LT1GQB3";

WebServer server(80);

// =====================
// PINS
// =====================
const int sensorPin = 34;
const int pumpPin   = 25;

// =====================
// CALIBRATION
// =====================
int dryValue = 3120;
int wetValue = 1340;

// =====================
// VARIABLES
// =====================
float smoothedMoisture = 0;
int currentMoisture = 0;

bool watering = false;
bool pumpRunning = false;
unsigned long lastToggle = 0;

const unsigned long pumpOnTime  = 6000;   // 6 seconds
const unsigned long pumpOffTime = 12000;  // 12 seconds

int cycleCount = 0;
const int maxCycles = 3;

bool manualOverride = false;

// =====================
// WEB SERVER
// =====================
void handleRoot() {
  String html = "<html><head><meta http-equiv='refresh' content='2'></head><body>";
  html += "<h1>Plant Monitor</h1>";
  html += "<p>Moisture: " + String(currentMoisture) + "%</p>";
  html += "<p>Mode: " + String(manualOverride ? "MANUAL" : "AUTO") + "</p>";
  html += "<p>Pump: " + String(pumpRunning ? "ON" : "OFF") + "</p>";

  html += "<a href='/on'><button>ON</button></a>";
  html += "<a href='/off'><button>OFF</button></a>";
  html += "<a href='/auto'><button>AUTO</button></a>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleOn() {
  manualOverride = true;
  pumpRunning = true;
  digitalWrite(pumpPin, HIGH);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleOff() {
  manualOverride = true;
  pumpRunning = false;
  digitalWrite(pumpPin, LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleAuto() {
  manualOverride = false;
  server.sendHeader("Location", "/");
  server.send(303);
}

// =====================
// SETUP
// =====================
void setup() {

  Serial.begin(115200);

  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, LOW);

  Wire.begin(21, 22);

  // OLED INIT
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
    while(true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0,0);
  display.println("Plant Monitor");
  display.display();
  delay(1500);

  // WIFI CONNECT (non-blocking timeout)
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  if(WiFi.status() == WL_CONNECTED){
    Serial.println("\nConnected");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi Failed");
  }

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/auto", handleAuto);

  server.begin();
}

// =====================
// LOOP
// =====================
void loop() {

  server.handleClient();

  int raw = analogRead(sensorPin);

  // basic sanity check
  if(raw < 100 || raw > 4000){
    Serial.println("Sensor issue");
    delay(1000);
    return;
  }

  int moisture = map(raw, dryValue, wetValue, 0, 100);
  moisture = constrain(moisture, 0, 100);

  // smoothing
  smoothedMoisture = (smoothedMoisture * 0.8) + (moisture * 0.2);
  currentMoisture = (int)smoothedMoisture;

  unsigned long now = millis();

  // AUTO MODE
  if(!manualOverride){

    if(!watering && currentMoisture < 30){
      watering = true;
      pumpRunning = true;
      cycleCount = 0;
      lastToggle = now;
      digitalWrite(pumpPin, HIGH);
    }

    if(watering && (cycleCount >= maxCycles || currentMoisture > 40)){
      watering = false;
      pumpRunning = false;
      digitalWrite(pumpPin, LOW);
    }

    if(watering){
      if(pumpRunning && now - lastToggle >= pumpOnTime){
        pumpRunning = false;
        lastToggle = now;
        digitalWrite(pumpPin, LOW);
        cycleCount++;
      }
      else if(!pumpRunning && now - lastToggle >= pumpOffTime){
        pumpRunning = true;
        lastToggle = now;
        digitalWrite(pumpPin, HIGH);
      }
    }
  }

  // =====================
  // OLED DISPLAY
  // =====================
  display.clearDisplay();

  display.setCursor(0,0);
  display.print("Moisture: ");
  display.print(currentMoisture);
  display.println("%");

  display.setCursor(0,16);
  display.print("Mode: ");
  display.println(manualOverride ? "MANUAL" : "AUTO");

  display.setCursor(0,32);
  display.print("Pump: ");
  display.println(pumpRunning ? "ON" : "OFF");

  display.display();

  // SERIAL DEBUG
  Serial.print("Raw: ");
  Serial.print(raw);
  Serial.print(" Moisture: ");
  Serial.print(currentMoisture);
  Serial.print("% Pump: ");
  Serial.println(pumpRunning ? "ON" : "OFF");

  delay(1000);
}