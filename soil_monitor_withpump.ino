#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int sensorPin = 34;
const int pumpPin   = 26;

int dryValue = 3106;
int wetValue = 1578;

LiquidCrystal_I2C lcd(0x27,16,2);

int pumpState = 0;
float smoothedMoisture = 0;

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
  digitalWrite(pumpPin, HIGH);   // relay OFF

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

  // smoothing filter
  smoothedMoisture = (smoothedMoisture * 0.7) + (moisture * 0.3);

  int stableMoisture = (int)smoothedMoisture;

  String condition = soilCondition(stableMoisture);

  // hysteresis pump control
  if(stableMoisture < 30 && pumpState == 0){
      digitalWrite(pumpPin, LOW);
      pumpState = 1;
  }

  if(stableMoisture > 45 && pumpState == 1){
      digitalWrite(pumpPin, HIGH);
      pumpState = 0;
  }

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

  delay(3000);
}