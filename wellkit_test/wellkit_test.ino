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
 
 HX711 Library (0.5.0)
  Rob Tillaart <rob.tillaart@gmail.com>
*/

#include <EEPROM.h> 
#include "HX711.h"

#define VERSION   101
#define DEBUG

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

#define HX711_DOUT  3
#define HX711_CLK   2
#define L298N_ENA   9 // PWM
#define L298N_IN1   8
#define L298N_IN2   7
#define MAG_IN      4

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

#define MOTOR_SPEED 250 //50          //128
#define MOTOR_DELAY_OPEN 230 //500         // 모터 open시 기본 이동 delay 
#define MOTOR_DELAY_CLOSE 110 //500         // 모터 close시 기본 이동 delay 
#define MOTOR_SLOW_DELAY 20     // 마지막 단계에서 모터 저속 운전을 위한 delay
#define MOTOR_CAL_DELAY 40    // 모터 위치 이동 보정을 위한 delay

#define MOTOR_JITTER_DELAY 100  // 모터 close시 센서 감지후 추가 이동을 위한 delay (with sensor)
#define MOTOR_LIMIT_CNT 30    // 모터 close시 센서 오류를 보정하기 위한 max 카운트 (with sensor)

#define LBS_TO_GRAM   (453.6)     // 미사용

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

struct wellkit {
  int version;
  float scala;
  long offSet;
};

struct wellkit data;

HX711 scale;

float factor = 246.84;    // calibration 전 기본값
long offset = -239664;    // calibration 전 기본값

int status;

void(* reset_func) (void) = 0;

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

char cmd;
void loop() 
{
  //while(1)
  {
    // put your main code here, to run repeatedly:
    if(Serial.available()){
      cmd = Serial.read();
#ifdef DEBUG 
      Serial.print("cmd : ");
      Serial.println(cmd);
      blink_builtin_led(100, 1);
#endif    

      switch(cmd)
      {
        case '0':
        {
          reset_func();
          break;
        }
        case '1':
        {
          // get weight from loadcell
          // load cell에서 무게 정보를 읽어와서 출력
          float weight = scale.get_units(5);
          //weight = weight * LBS_TO_GRAM;
          Serial.println(weight, 1);
          break;
        }
        case '2':
        {
          // get calibration value (scala factor)
          Serial.println(data.scala);
          break;
        }
        case '3':
        {
          // get calibration value (offset)
          Serial.println(data.offSet);
          break;
        }
        case '4':
        {
          // step 1 : calibration with known weight (1단계 : 저울에 물체를 모두 치우고 명령 수행)
          scale.tare();
          data.offSet=scale.get_units(10);
          Serial.println("OK");
          break;
        }
        case '5':
        {
          // step 2 : calibration with known weight (2단계 : 저울에 물체를 올리고 명령 수행)
          int value = Serial.parseInt();
          proc_calibration(value);
          float weight = scale.get_units(5);
          Serial.println(weight, 1);
          break;
        }
        case '6':
        {
          // Open Cover
          motor_open();
          Serial.println("OK");
          break;
        }
        case '7':
        {
          // Close Cover
          motor_close();
          Serial.println("OK");
          break;
        }
        case 'm': case 'M':
        {
          // Open Cover with sensor
          motor_close_sensor();
          Serial.println("OK");
          break;
        }
        case '8':
        {
          // Get Cover status
          if(check_door_sensor())
            Serial.println("0");  // close
          else
            Serial.println("1");  // open

          break;
        }
        case '9':
        {
          // get zero factor value for scale
          long zero_factor = scale.read_average();
          Serial.println(zero_factor);
          break;
        }
        case 'a': case 'A':
        {
          // Open motor 1-step
          motor_open_step();
          break;
        }
        case 'b': case 'B':
        {
          // Close motor 1-step
          motor_close_step();
          break;
        }
#ifdef DEBUG       
        case 'c': case 'C':
        {
          // Test routine for motor and sensor
          test_motor_sensor();
          break;
        }
#endif      
#ifdef DEBUG  
        case 'x': case 'X':
        {
          // Test routine for scale
          test_proc_calibration();
          break;
        }
#endif
        default:
#ifdef DEBUG  
          Serial.println("error");
#endif
          break;
      }

    }
    else
    {
      //Serial.println("serial error");
    }
    delay(10);
  }
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

// return HIGH(1) if closed else return LOW(0)
int check_door_sensor(void)
{
#if 1  
  for(int i=0;i<7;i++)
  {
    if(digitalRead(MAG_IN)==LOW)
      return 0;
    delay(1);
  }

  return 1;
#else  
  return digitalRead(MAG_IN);
#endif  
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void motor_init(void)
{
  pinMode(L298N_ENA, OUTPUT);
  pinMode(L298N_IN1, OUTPUT);
  pinMode(L298N_IN2, OUTPUT);

  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, LOW);
  analogWrite(L298N_ENA, 0);
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void motor_open_step(void)
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

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void motor_close_step(void)
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

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void motor_open(void)
{
#ifdef DEBUG 
  Serial.println("motor_open(): OK");
#endif
  digitalWrite(L298N_IN1, HIGH);
  digitalWrite(L298N_IN2, LOW);
  analogWrite(L298N_ENA, MOTOR_SPEED);
  delay(MOTOR_DELAY_OPEN);
  for (int i = MOTOR_SPEED; i > 0; i--)
  {
    analogWrite(L298N_ENA, i);
    delay(MOTOR_SLOW_DELAY);
  }
  analogWrite(L298N_ENA, 0);
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void motor_close(void)
{
#ifdef DEBUG 
  Serial.println("motor_close(): OK");
#endif
  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, HIGH);
  analogWrite(L298N_ENA, MOTOR_SPEED);
  delay(MOTOR_DELAY_CLOSE);
  for (int i = MOTOR_SPEED; i > 0; i--)
  {
    analogWrite(L298N_ENA, i);
    delay(MOTOR_SLOW_DELAY);
  }
  analogWrite(L298N_ENA, 0);
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void motor_close_sensor(void)
{
#ifdef DEBUG 
  Serial.println("motor_close_sensor(): OK");
#endif
  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, HIGH);
  analogWrite(L298N_ENA, MOTOR_SPEED);

  int cnt=0;
  while(cnt++ < MOTOR_LIMIT_CNT)
  {
    delay(50);
    if(check_door_sensor())
    {
      delay(MOTOR_JITTER_DELAY);
      break;
    }    
  }
  analogWrite(L298N_ENA, 0);
#ifdef DEBUG 
  Serial.print("motor_close_sensor(): cnt = ");
  Serial.println(cnt);
#endif  
}
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void save_struct(int addr, struct wellkit value) {
  value.version = VERSION;
  EEPROM.put(addr, value);
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

struct wellkit load_struct(int addr) {
  EEPROM.get( addr, data );
  return data;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void proc_calibration(int weight)
{
    scale.calibrate_scale(weight, 5);
    data.scala=scale.get_units(10);

    data.offSet = scale.get_offset();
    data.scala = scale.get_scale();
    save_struct(0, data);
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

#ifdef DEBUG 

void test_motor_sensor(void)
{
#ifdef DEBUG 
  Serial.println("motor_close(): OK");
#endif

  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, HIGH);
  analogWrite(L298N_ENA, MOTOR_SPEED);

  while(1)
  {
    if(check_door_sensor())
    {
      analogWrite(L298N_ENA, 0);
    }
    else
    {
      analogWrite(L298N_ENA, MOTOR_SPEED);
    }
  }
  analogWrite(L298N_ENA, 0);
}


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
