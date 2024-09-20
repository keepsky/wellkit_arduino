/*
 Wellkit project
 By: Joonho Park
 Sungkyunkwan University
 Date: July 19th, 2024
 License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 Github : https://github.com/keepsky/wellkit_arduino/tree/main
 
 Arduino pin 
   D2 -> HX711 CLK
   D3 -> HX711 DOUT
   5V -> VCC 

   D7 -> L298N IN2
   D8 -> L298N IN1
   D9 -> L298N ENA (PWM)
   5V -> L298N_5V (note) or VIN <- L298N_5V   
   GND -> GND (from external power source) 
 
*/

#include <EEPROM.h> 
#include "HX711.h"

#define DEBUG

#define HX711_DOUT  3
#define HX711_CLK   2
#define L298N_ENA   9 // PWM
#define L298N_IN1   8
#define L298N_IN2   7

#define MOTOR_SPEED 255
#define MOTOR_DELAY 500

#define ADDR_CALIBRATION  0
#define ADDR_CAL_STATUS   10

#define CAL_STATUS_NONE   0
#define CAL_STATUS_OK     0xAA

#define LBS_TO_GRAM   (453.6)


HX711 scale;

float calibration_factor = 120000;
int status;


void setup() 
{
  Serial.begin(9600);
  delay(2000);

  pinMode(LED_BUILTIN, OUTPUT);

  motor_init();

  scale.begin(HX711_DOUT, HX711_CLK);

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
      float weight = scale.get_units(5);
      weight = weight * LBS_TO_GRAM;
      Serial.println(weight, 1);

    // get calibration value
    } else if (cmd == '2'){         
      Serial.println(calibration_factor);

    } else if (cmd == '3'){         
      scale.set_scale();
      scale.tare(); //Reset the scale to 0
#ifdef DEBUG  
      Serial.println("OK");
#endif
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
      motor_open();
#ifdef DEBUG  
      Serial.println("Open Cover");
#endif
    // Close Cover
    } else if (cmd == '6'){         
      motor_close();
#ifdef DEBUG  
      Serial.println("Close Cover");
#endif
    // Get Cover status
    } else if (cmd == '7'){         
#ifdef DEBUG  
      Serial.println("get cover status");
#endif
    // get zero factor value
    } else if (cmd == '9'){         
      long zero_factor = scale.read_average();
      Serial.println(zero_factor);

    // otherwise
    } else {
#ifdef DEBUG  
      Serial.println("error");
#endif
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

void motor_init(void)
{
  pinMode(L298N_ENA, OUTPUT);
  pinMode(L298N_IN1, OUTPUT);
  pinMode(L298N_IN2, OUTPUT);

  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, LOW);
  analogWrite(L298N_ENA, 0);
}

void motor_open(void)
{
  digitalWrite(L298N_IN1, HIGH);
  digitalWrite(L298N_IN2, LOW);
  analogWrite(L298N_ENA, MOTOR_SPEED);
  delay(MOTOR_DELAY);
  analogWrite(L298N_ENA, 0);
  delay(MOTOR_DELAY);  
}

void motor_close(void)
{
  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, HIGH);
  analogWrite(L298N_ENA, MOTOR_SPEED);
  delay(MOTOR_DELAY);
  analogWrite(L298N_ENA, 0);
  delay(MOTOR_DELAY);  
}



