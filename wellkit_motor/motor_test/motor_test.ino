

#define L298N_ENA   9 // PWM
#define L298N_IN1   8
#define L298N_IN2   7

int motor_speed = 255;
int motor_delay = 500;

void setup() 
{ 
  Serial.begin(9600);
  delay(2000);

  motor_init();
} 

char cmd;
void loop()
{
  // put your main code here, to run repeatedly:
  if(Serial.available()){
    cmd = Serial.read();
    if (cmd == '1') {               
      motor_open();

    } else if (cmd == '2'){         
      motor_close();

    } else if (cmd == '3'){         
      int value = Serial.parseInt();
      if(value >= 0 && value <= 255) {
        motor_speed = value;        
        Serial.println("OK"); 
      } else {
        Serial.println("error"); 
      }

    } else if (cmd == '4'){         
      int value = Serial.parseInt();
      if(value >= 0 && value <= 10000) {
        motor_delay = value;        
        Serial.println("OK"); 
      } else {
        Serial.println("error"); 
      }
    } else {
      Serial.println("error");      
    }
  }
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
  analogWrite(L298N_ENA, motor_speed);
  delay(motor_delay);
  analogWrite(L298N_ENA, 0);
  delay(motor_delay);  
}

void motor_close(void)
{
  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, HIGH);
  analogWrite(L298N_ENA, motor_speed);
  delay(motor_delay);
  analogWrite(L298N_ENA, 0);
  delay(motor_delay);  
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

#define ENA   9 // PWM
#define IN1   8
#define IN2   7
void loop_test() 
{ 
  // DC모터 A 회전  
  digitalWrite(IN1, HIGH); // 디지털 10번 핀에 디지털 신호 HIGH 출력
  digitalWrite(IN2, LOW); // 디지털 9번 핀에 디지털 신호 LOW 출력
  analogWrite(ENA, 255); // 디지털 11번 핀에 PWM 아날로그 신호 5V 출력
  delay(1000); // 1000ms 대기  
  analogWrite(ENA, 0); // 디지털 11번 핀에 PWM 아날로그 신호 0V 출력
  delay(1000);  
  // DC모터 A 역회전
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENA, 255);
  delay(1000);
  analogWrite(ENA, 0);
  delay(1000);

  
  // DC모터 A 회전 가속
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  for (int i = 1; i <= 255; i++) // 10 ms 간격으로 점점 빠르게 회전
  {
    analogWrite(ENA, i);
    delay(10); // 10 ms 대기
  }
  delay(1000);
  // DC모터 A 회전 감속
  for (int i = 254; i >= 0; i--) // 10 ms 간격으로 점점 느리게 회전
  {
    analogWrite(ENA, i);
    delay(10);
  }
  delay(1000);
  // DC모터 A 역회전 가속  
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  for (int i = 1; i <= 255; i++)
  {
    analogWrite(ENA, i);
    delay(10);
  }
  delay(1000);
  // DC모터 A 역회전 감속 
  for (int i = 254; i >= 0; i--)
  {
    analogWrite(ENA, i);
    delay(10);
  }
  delay(1000); 
  
  
}


