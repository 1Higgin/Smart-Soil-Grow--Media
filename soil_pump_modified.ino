#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int sensorPin = 34;
const int pumpPin   = 25;   // BC547 driver to relay IN

int dryValue = 3106;
int wetValue = 1578;

LiquidCrystal_I2C lcd(0x27,16,2);

float smoothedMoisture = 0;

// --- Pump cycle control ---
bool watering = false;
bool pumpRunning = false;
unsigned long lastToggle = 0;

const unsigned long pumpOnTime  = 6000;   // realistic 6 sec ON
const unsigned long pumpOffTime = 12000;   // realistic 12 sec OFF

int cycleCount = 0;
const int maxCycles = 3;

// --- Read soil and smooth ---
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

void setup(){
  Serial.begin(115200);

  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, LOW);   // pump OFF initially

  Wire.begin(21,22);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0,0);
  lcd.print("Plant Monitor");
  delay(2000);
  lcd.clear();
}

void loop(){
  int raw = readSoil();

  int moisture = map(raw, dryValue, wetValue, 0, 100);
  moisture = constrain(moisture,0,100);

  smoothedMoisture = (smoothedMoisture * 0.7) + (moisture * 0.3);
  int stableMoisture = (int)smoothedMoisture;

  String condition = soilCondition(stableMoisture);

  unsigned long now = millis();

  // --- Start watering if below 30% and not already watering ---
  if(!watering && stableMoisture < 30){
    watering = true;
    pumpRunning = true;
    cycleCount = 0;
    lastToggle = now;
    digitalWrite(pumpPin, HIGH); // pump ON
    Serial.println("Watering started");
  }

  // --- Stop watering after max cycles or above 40% ---
  if(watering && (cycleCount >= maxCycles || stableMoisture > 40)){
    watering = false;
    pumpRunning = false;
    digitalWrite(pumpPin, LOW); // pump OFF
    Serial.println("Watering completed");
  }

  // --- Intermittent 3-cycle control ---
  if(watering){
    if(pumpRunning && (now - lastToggle >= pumpOnTime)){
      pumpRunning = false;
      lastToggle = now;
      digitalWrite(pumpPin, LOW);  // pump OFF
      cycleCount++;
      Serial.print("Pump OFF, cycle ");
      Serial.println(cycleCount);
    }
    else if(!pumpRunning && (now - lastToggle >= pumpOffTime)){
      if(cycleCount < maxCycles && stableMoisture < 40){
        pumpRunning = true;
        lastToggle = now;
        digitalWrite(pumpPin, HIGH); // pump ON
        Serial.print("Pump ON, cycle ");
        Serial.println(cycleCount + 1);
      }
    }
  }

  // --- Display ---
  Serial.print("Moisture: ");
  Serial.print(stableMoisture);
  Serial.print("%  ");
  Serial.println(condition);

  lcd.setCursor(0,0);
  lcd.print("Moisture:");
  lcd.print(stableMoisture);
  lcd.print("%   ");

  lcd.setCursor(0,1);
  lcd.print("Soil:");
  lcd.print(condition);
  lcd.print("     ");

  delay(500); // short delay for display stability
}
