#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int sensorPin = 34;

int dryValue = 3106;
int wetValue = 1578;

LiquidCrystal_I2C lcd(0x27,16,2);

int readSoil(){

  long total = 0;

  for(int i=0;i<10;i++){
    total += analogRead(sensorPin);
    delay(5);
  }

  return total / 10;
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

  Wire.begin(21,22);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0,0);
  lcd.print("Soil Monitor");
  delay(2000);
}

void loop(){

  int raw = readSoil();

  int moisture = map(raw, dryValue, wetValue, 0, 100);
  moisture = constrain(moisture,0,100);

  String condition = soilCondition(moisture);

  Serial.print("Moisture: ");
  Serial.print(moisture);
  Serial.print("%  ");
  Serial.println(condition);

  lcd.setCursor(0,0);
  lcd.print("Moisture:");
  lcd.print(moisture);
  lcd.print("%   ");

  lcd.setCursor(0,1);
  lcd.print("Soil:");
  lcd.print(condition);
  lcd.print("      ");

  delay(2000);
}