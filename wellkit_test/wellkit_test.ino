/*
 Wellkit project
 By: Joonho Park
 Sungkyunkwan University
 Date: July 19th, 2024
 License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 Github : https://github.com/keepsky/wellkit_arduino/tree/main
 
 Arduino pin 
   2 -> HX711 CLK
   3 -> DOUT
   5V -> VCC
   GND -> GND 
 
*/


#include <EEPROM.h> 
#include "HX711.h"

#define DEBUG

#define ADDR_CALIBRATION  0
#define ADDR_CAL_STATUS   10

#define CAL_STATUS_NONE   0
#define CAL_STATUS_OK     0xAA

#define LBS_TO_GRAM   (453.6)


#define DOUT  3
#define CLK   2

HX711 scale;

float calibration_factor = 120000;
int status;

void setup() 
{
  Serial.begin(9600);
  delay(2000);

  pinMode(LED_BUILTIN, OUTPUT);

  scale.begin(DOUT, CLK);

  status = EEPROM.read(ADDR_CAL_STATUS);

#ifdef DEBUG  
  Serial.print("status : ");
  Serial.println(status);
#endif

  if(status == CAL_STATUS_OK)
  {
    scale.set_scale();
    scale.tare(); //Reset the scale to 0

    // read calibration value from EEPROM
    calibration_factor = get_eeprom_calibration();  
    scale.set_scale(calibration_factor);  //This value is obtained by using calibration step
  }
}

char cmd;
void loop() 
{
  // put your main code here, to run repeatedly:
  if(Serial.available()){
    cmd = Serial.read();
    blink_builtin_led(100, 1);


    // get weight
    if (cmd == '1') {               
      // load cell에서 무게 정보를 읽어와서 출력하는 코드 필요
      blink_builtin_led(50, 1);
      float weight = scale.get_units(5);
      weight = weight * LBS_TO_GRAM;
      Serial.println(weight, 1);

    // get calibration value
    } else if (cmd == '2'){         
      Serial.println(calibration_factor);

    } else if (cmd == '3'){         
      scale.set_scale();
      scale.tare(); //Reset the scale to 0
      Serial.println("OK");

    // set calibration value
    } else if (cmd == '4'){         
      float value = Serial.parseFloat();
      calibration_factor = value;
      set_eeprom_calibration(calibration_factor);
      scale.set_scale();
      scale.tare(); //Reset the scale to 0
      scale.set_scale(calibration_factor); //Adjust to this calibration factor
      Serial.println(calibration_factor);


    // Open Cover
    } else if (cmd == '5'){         
      Serial.println("Open Cover");

    // Close Cover
    } else if (cmd == '6'){         
      Serial.println("Close Cover");

    // Get Cover status
    } else if (cmd == '7'){         
      Serial.println("get cover status");


    // get zero factor value
    } else if (cmd == '9'){         
      long zero_factor = scale.read_average();
      Serial.println(zero_factor);


    // otherwise
    } else {
      Serial.println("error");


    }
    delay(100);
  }
}

void blink_builtin_led(int duration, int num)
{
  for(int i=0;i<num;i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(duration);               
    digitalWrite(LED_BUILTIN, LOW);   
    delay(duration);               
  }
}

float get_eeprom_calibration()
{
  float val;
  char *p = (char*)&val;

  for(int i=0;i<8;i++)
  {
    *p++ = EEPROM.read(i);
  }

  return val;
}

void set_eeprom_calibration(float val)
{
  char *p = (char*)&val;

  for(int i=0;i<8;i++)
  {
    EEPROM.write(i, *p++);
  }

  EEPROM.write(ADDR_CAL_STATUS, CAL_STATUS_OK);
}



