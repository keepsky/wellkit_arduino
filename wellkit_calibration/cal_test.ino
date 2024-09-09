#include <EEPROM.h> 
#include "HX711.h"

#define LBS_TO_GRAM   (453.6)

#define DOUT  3
#define CLK  2

HX711 scale;

float calibration_factor = 120000; //-7050; //-7050 worked for my 440lb max scale setup

void setup() {
  
  scale.begin(DOUT, CLK);

  Serial.begin(9600);
  Serial.println("HX711 calibration sketch");
  Serial.println("Remove all weight from scale");
  Serial.println("After readings begin, place known weight on scale");
  Serial.println("Press + or a to increase calibration factor");
  Serial.println("Press - or z to decrease calibration factor");

  scale.set_scale();
  scale.tare(); //Reset the scale to 0

  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);
}

void loop() {

  float value;

  scale.set_scale(calibration_factor); //Adjust to this calibration factor

  Serial.print("Reading: ");

  value = scale.get_units()*LBS_TO_GRAM;
  Serial.print(value, 1);
  Serial.print(" g"); //Change this to kg and re-adjust the calibration factor if you follow SI units like a sane person
  Serial.print(" calibration_factor: ");
  Serial.print(calibration_factor);
  Serial.println();

  if(Serial.available())
  {
    char temp = Serial.read();
    if(temp == '+')
      calibration_factor += 10;
    else if(temp == '-')
      calibration_factor -= 10;
    else if(temp == 'a')
      calibration_factor += 1000;
    else if(temp == 'z')
      calibration_factor -= 1000;
    else if(temp == 's')
      set_eeprom(calibration_factor);
    else if(temp == 'l')
      calibration_factor=get_eeprom();
  }
}


float get_eeprom(void)
{
  float val;
  char *p = (char*)&val;

  for(int i=0;i<8;i++)
  {
    *p++ = EEPROM.read(i);
  }

  return val;
}

void set_eeprom(float val)
{
  char *p = (char*)&val;

  for(int i=0;i<8;i++)
  {
    EEPROM.write(i, *p++);
  }
}

