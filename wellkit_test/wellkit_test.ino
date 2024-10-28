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

   D4 -> Magnetic(OUT)
   5V -> Magnetic(IN)
 
*/

#include <EEPROM.h> 
#include "HX711.h"

#define VERSION   101
#define DEBUG

#define HX711_DOUT  3
#define HX711_CLK   2
#define L298N_ENA   9 // PWM
#define L298N_IN1   8
#define L298N_IN2   7
#define MAG_IN      4


#define MOTOR_SPEED 50 //128
#define MOTOR_DELAY 500
#define MOTOR_DELAY_DELTA 40
#define MOTOR_SLOW_DELAY 20
#define MOTOR_CAL_DELAY 40

#define LBS_TO_GRAM   (453.6)

struct wellkit {
  int version;
  float scala;
  long offSet;
};

struct wellkit data;

HX711 scale;

float factor = 246.84;
long offset = -239664;

int status;

void(* reset_func) (void) = 0;

void setup() 
{
  Serial.begin(9600);
  delay(2000);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MAG_IN, INPUT);

  motor_init();

  scale.begin(HX711_DOUT, HX711_CLK);

  data = load_struct(0);

  if(data.version == VERSION)
  {
#ifdef DEBUG  
    Serial.print("status : ");
    Serial.println(status);
    Serial.println("Calibration value is valid : ");
    Serial.print("scala : ");
    Serial.println(data.scala);
    Serial.print("offset : ");
    Serial.println(data.offSet);
#endif

    scale.set_offset(data.offSet);
    scale.set_scale(data.scala);
  }
#ifdef DEBUG  
  else
  {
    Serial.println("Calibration value is invalid. you need to go through calibration");
  }
#endif
}

char cmd;
void loop() 
{
  // put your main code here, to run repeatedly:
  if(Serial.available()){
    cmd = Serial.read();
#ifdef DEBUG 
    blink_builtin_led(100, 1);
#endif    

    // get weight
    if (cmd == '1') {               
      // load cell에서 무게 정보를 읽어와서 출력하는 코드 필요
      float weight = scale.get_units(5);
      //weight = weight * LBS_TO_GRAM;
      Serial.println(weight, 1);

    // get calibration value (scala)
    } else if (cmd == '2'){         
      Serial.println(data.scala);

    // get calibration value (offset)
    } else if (cmd == '3'){         
      Serial.println(data.offSet);

    // step 1 : calibration with known weight
    } else if (cmd == '4'){         
      scale.tare();
      data.offSet=scale.get_units(10);
      Serial.println("OK");

    // step 2 : calibration with known weight
    } else if (cmd == '5'){         
      int value = Serial.parseInt();
      proc_calibration(value);
      float weight = scale.get_units(5);
      Serial.println(weight, 1);

    // Open Cover
    } else if (cmd == '6'){        
      motor_open();
      Serial.println("OK");

    // Close Cover
    } else if (cmd == '7'){         
      motor_close();
      Serial.println("OK");

    // Get Cover status
    } else if (cmd == '8'){         
#ifdef DEBUG  
      Serial.println("get cover status");
#endif

    // get zero factor value
    } else if (cmd == '9'){         
      long zero_factor = scale.read_average();
      Serial.println(zero_factor);

    // reset board
    } else if (cmd == '0'){         
      reset_func();

    } else if (cmd == 'a' || cmd == 'A'){         
      motor_cal_open();

    } else if (cmd == 'b' || cmd == 'B'){         
      motor_cal_close();

#ifdef DEBUG  
    // get zero factor value
    } else if (cmd == 'x' || cmd == 'X'){         
      test_proc_calibration();
#endif
    // otherwise
    } else {
#ifdef DEBUG  
      Serial.println("error");
#endif
    }
    delay(100);
  }
}

#ifdef DEBUG 
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
#endif

int check_door_sensor(void)
{
  int sw = digitalRead(MAG_IN);
  if(sw == HIGH)
    return 1;

  return 0;
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

void motor_cal_open(void)
{
#ifdef DEBUG 
  Serial.println("motor_cal_open(): OK");
#endif
  digitalWrite(L298N_IN1, HIGH);
  digitalWrite(L298N_IN2, LOW);
  analogWrite(L298N_ENA, MOTOR_SPEED);
  delay(MOTOR_CAL_DELAY);
  analogWrite(L298N_ENA, 0);
  digitalWrite(L298N_IN1, LOW);
}

void motor_cal_close(void)
{
#ifdef DEBUG 
  Serial.println("motor_cal_close(): OK");
#endif
  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, HIGH);
  analogWrite(L298N_ENA, MOTOR_SPEED);
  delay(MOTOR_CAL_DELAY);
  analogWrite(L298N_ENA, 0);
  digitalWrite(L298N_IN2, LOW);
}

void motor_open(void)
{
#ifdef DEBUG 
  Serial.println("motor_open(): OK");
#endif
  digitalWrite(L298N_IN1, HIGH);
  digitalWrite(L298N_IN2, LOW);
  analogWrite(L298N_ENA, MOTOR_SPEED);
  delay(MOTOR_DELAY);
  for (int i = MOTOR_SPEED; i > 0; i--)
  {
    analogWrite(L298N_ENA, i);
    delay(MOTOR_SLOW_DELAY);
  }
  analogWrite(L298N_ENA, 0);
}

void motor_close(void)
{
#ifdef DEBUG 
  Serial.println("motor_close(): OK");
#endif
  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, HIGH);
  analogWrite(L298N_ENA, MOTOR_SPEED);
  delay(MOTOR_DELAY + MOTOR_DELAY_DELTA);
  for (int i = MOTOR_SPEED; i > 0; i--)
  {
    analogWrite(L298N_ENA, i);
    delay(MOTOR_SLOW_DELAY);
    if(check_door_sensor() == 1)
      break;
  }
  analogWrite(L298N_ENA, 0);
}


void save_struct(int addr, struct wellkit value) {
  value.version = VERSION;
  EEPROM.put(addr, value);
}

struct wellkit load_struct(int addr) {
  EEPROM.get( addr, data );
  return data;
}


void proc_calibration(int weight)
{
    scale.calibrate_scale(weight, 5);
    data.scala=scale.get_units(10);

    data.offSet = scale.get_offset();
    data.scala = scale.get_scale();
    save_struct(0, data);
}


#ifdef DEBUG 

#define OBJECT  500

void test_proc_calibration(void)
{
    Serial.print("UNITS: ");
    Serial.println(scale.get_units(10));

    Serial.println("\nEmpty the scale, press a key to continue");
    while (!Serial.available());
    while (Serial.available()) Serial.read();

    scale.tare();
    Serial.print("UNITS: ");
    data.offSet=scale.get_units(10);
    Serial.println(data.offSet);


    Serial.println("\nPut 1000 gram in the scale, press a key to continue");
    while (!Serial.available());
    while (Serial.available()) Serial.read();

    scale.calibrate_scale(OBJECT, 5);
    Serial.print("UNITS: ");
    data.scala=scale.get_units(10);
    Serial.println(data.scala);

    Serial.println("\nScale is calibrated, your calibration values:");

    long scaleOffset = scale.get_offset();
    Serial.print("\nOffset \t");
    Serial.println(scaleOffset);

    float scaleFactor = scale.get_scale();
    Serial.print("Scale \t");
    Serial.println(scaleFactor);

    Serial.println("\nUse this code for setting zero and calibration factor permanently:");

    Serial.print("\nscale.set_offset(");
    Serial.print(scaleOffset);
    Serial.println(");");
    Serial.print("scale.set_scale(");
    Serial.print(scaleFactor);
    Serial.println(");");

    Serial.println("\nPress a key to continue");
    while (!Serial.available());
    while (Serial.available()) Serial.read();

//scale.set_offset(-88627);
//scale.set_scale(101.05);

    data.scala=scaleFactor;
    data.offSet=scaleOffset; 
    save_struct(0, data);//Save to eeprom   
}
#endif
