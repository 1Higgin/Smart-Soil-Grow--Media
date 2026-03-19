#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int sensorPin = 34;
const int pumpPin   = 26;

int dryValue = 3106;
int wetValue = 1578;

LiquidCrystal_I2C lcd(0x27,16,2);

// --- Pump + watering control ---
bool watering = false;
bool pumpRunning = false;

unsigned long lastToggle = 0;

const unsigned long pumpOnTime  = 2000;   // 2 sec ON
const unsigned long pumpOffTime = 10000;  // 10 sec OFF

// --- Moisture ---
float smoothedMoisture = 0;

// --- Timing ---
unsigned long lastReadTime = 0;
const unsigned long readInterval = 500;   // sensor read every 0.5 sec

// -----------------------------

int readSoil() {
  long total = 0;

  for(int i=0;i<20;i++){
    total += analogRead(sensorPin);
    delay(3);
  }

  return total / 20;
}

String soilCondition(int moisture){
  if(moisture <= 15) return "VERY DRY";
  else if(moisture <= 35) return "DRY";
  else if(moisture <= 60) return "MOIST";
  else if(moisture <= 80) return "WET";
  else return "VERY WET";
}

// -----------------------------

void setup(){
  Serial.begin(115200);

  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, HIGH); // relay OFF

  Wire.begin(21,22);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0,0);
  lcd.print("Plant Monitor");
  delay(2000);
  lcd.clear();
}

// -----------------------------

void loop(){

  unsigned long now = millis();

  // --- READ SENSOR (timed) ---
  if(now - lastReadTime >= readInterval){
    lastReadTime = now;

    int raw = readSoil();

    int moisture = map(raw, dryValue, wetValue, 0, 100);
    moisture = constrain(moisture,0,100);

    // smoothing
    smoothedMoisture = (smoothedMoisture * 0.7) + (moisture * 0.3);
  }

  int stableMoisture = (int)smoothedMoisture;
  String condition = soilCondition(stableMoisture);

  // --- START watering ---
  if(!watering && stableMoisture < 30){
    watering = true;
    pumpRunning = true;
    lastToggle = now;
    digitalWrite(pumpPin, LOW); // ON
  }

  // --- STOP watering ---
  if(watering && stableMoisture > 40){
    watering = false;
    pumpRunning = false;
    digitalWrite(pumpPin, HIGH); // OFF
  }

  // --- INTERMITTENT PUMP ---
  if(watering){
    if(pumpRunning && (now - lastToggle >= pumpOnTime)){
      pumpRunning = false;
      lastToggle = now;
      digitalWrite(pumpPin, HIGH); // OFF
    }
    else if(!pumpRunning && (now - lastToggle >= pumpOffTime)){
      pumpRunning = true;
      lastToggle = now;
      digitalWrite(pumpPin, LOW); // ON
    }
  }

  // --- SERIAL OUTPUT ---
  Serial.print("Moisture: ");
  Serial.print(stableMoisture);
  Serial.print("%  ");
  Serial.println(condition);

  // --- LCD ---
  lcd.setCursor(0,0);
  lcd.print("Moisture:");
  lcd.print(stableMoisture);
  lcd.print("%   ");

  lcd.setCursor(0,1);
  lcd.print("Soil:");
  lcd.print(condition);
  lcd.print("     ");
}