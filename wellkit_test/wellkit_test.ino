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

//#define CALIBRATION_MODE
#define LBS_TO_GRAM   (453.6)

#define DOUT  3
#define CLK   2
HX711 scale(DOUT, CLK);

long calibration_factor = -14;

#ifdef CALIBRATION_MODE
float units;
float ounces;
#endif

void setup() 
{
  // read calibration value from EEPROM
  calibration_factor = get_eeprom();

  pinMode(LED_BUILTIN, OUTPUT);
  delay(2000);
  Serial.begin(9600);
  delay(2000);
  //while(!Serial) ;    // 연결을 기다립니다.
  scale.set_scale(calibration_factor);  //This value is obtained by using calibration step
  scale.tare();   //Assuming there is no weight on the scale at start up, reset the scale to 0

#ifdef CALIBRATION_MODE
  Serial.println("HX711 scale test");
  Serial.print("factor : ");
  Serial.println(calibration_factor);

  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);
#endif

}

char cmd;
void loop() 
{
#ifdef CALIBRATION_MODE
  calibration_mode();
#endif

  // put your main code here, to run repeatedly:
  if(Serial.available()){
    cmd = Serial.read();
    blink_builtin_led(100, 1);


    // get weight
    if (cmd == '1') {               
      // load cell에서 무게 정보를 읽어와서 출력하는 코드 필요
      blink_builtin_led(50, 1);
      int weight = scale.get_units();
      weight = (float)weight * LBS_TO_GRAM;
      Serial.println(weight, 1);


    // moving trailer
    } else if (cmd == '2'){         
      blink_builtin_led(50, 3);
      Serial.println("moving trailer");
      // TODO : Relay 스위치를 통해서 모터를 구동시키는 코드 필요


    // get calibration value
    } else if (cmd == '3'){         
      Serial.println(calibration_factor);


    // set calibration value
    } else if (cmd == '4'){         
      int cal = Serial.parseInt();
      calibration_factor = cal;
      set_eeprom(calibration_factor);
      scale.set_scale(calibration_factor); //Adjust to this calibration factor
      //scale.tare();
      //Serial.println(calibration_factor);


    // 현재 calibration 값으로 영점 조정(tare)
    // 무게 센서 위에 아무것도 올려 놓지 않은 상태에서 진행해야함
    } else if (cmd == '5'){         
      scale.set_scale(calibration_factor); //Adjust to this calibration factor
      scale.tare();


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

long get_eeprom(void)
{
  long val;
  char *p = (char*)&val;

  for(int i=0;i<8;i++)
  {
    *p++ = EEPROM.read(i);
  }

  return val;
}

void set_eeprom(long val)
{
  char *p = (char*)&val;

  for(int i=0;i<8;i++)
  {
    EEPROM.write(i, *p++);
  }
}



#ifdef CALIBRATI1ON_MODE
void calibration_mode(void)
{
  while(1)
  {
    scale.set_scale(calibration_factor); //Adjust to this calibration factor

    Serial.print("Reading: ");
    units = scale.get_units(), 10;
    if (units < 0)
    {
      units = 0.00;
    }
    ounces = units * 0.035274;
    Serial.print(units);
    Serial.print(" grams"); 
    Serial.print(" calibration_factor: ");
    Serial.print(calibration_factor);
    Serial.println();

    if(Serial.available())
    {
      char temp = Serial.read();
      if(temp == '+')
        calibration_factor += 1;
      else if(temp == '-')
        calibration_factor -= 1;
      else if(temp == 'a')
        calibration_factor += 10;
      else if(temp == 'z')
        calibration_factor -= 10;
    }
  }
}
#endif
