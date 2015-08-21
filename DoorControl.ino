
#include <Bounce.h>
#include <MuxShield.h>

#define BUTTON_ROW 1
#define HALL0_PIN 0
#define HALL1_PIN 1
#define HALL2_PIN 2
#define HALL3_PIN 3
#define OK_PIN 4
#define GO_PIN 5

#define MOTOR_PIN 5
#define DIR_PIN A3

bool goState = LOW;
bool slowState = LOW;
bool dir = HIGH;

int goSpeed;
int slowSpeed;
int doorState;
int speed;

int upGoSpeed = 150;
int upSlowSpeed = 50;
int downGoSpeed = 75;
int downSlowSpeed = 35;

MuxShield muxShield;

Bounce bounceHall0 = Bounce();
Bounce bounceHall1 = Bounce();
Bounce bounceHall2 = Bounce();
Bounce bounceHall3 = Bounce();
Bounce bounceOK = Bounce();
Bounce bounceGO = Bounce();

void setup() {

  muxShield.setMode(BUTTON_ROW, DIGITAL_IN_PULLUP);
  
  bounceHall0.attach(BUTTON_ROW, HALL0_PIN);
  bounceHall1.attach(BUTTON_ROW, HALL1_PIN);
  bounceHall2.attach(BUTTON_ROW, HALL2_PIN);
  bounceHall3.attach(BUTTON_ROW, HALL3_PIN);
  bounceOK.attach(BUTTON_ROW, OK_PIN);
  bounceGO.attach(BUTTON_ROW, GO_PIN);
  
  bounceHall0.interval(50);
  bounceHall1.interval(50);
  bounceHall2.interval(10);
  bounceHall3.interval(10);
  bounceOK.interval(50);
  bounceGO.interval(50);

  pinMode(MOTOR_PIN,OUTPUT);
  pinMode(DIR_PIN,OUTPUT);
  analogWrite(MOTOR_PIN,0);
  analogWrite(DIR_PIN,0);
  
  Serial.begin(115200);

}

void loop() {
  
  bounceHall0.update();
  bounceHall1.update();
  bounceHall2.update();
  bounceHall3.update();
  bounceOK.update();
  bounceGO.update();

  digitalWrite(DIR_PIN, dir);

  if ( !bounceOK.read() ) {
    if ( !bounceGO.read() ) {
      dir = HIGH;
      digitalWrite(DIR_PIN, dir);
      goSpeed = upGoSpeed;
      slowSpeed = upSlowSpeed;
      
      if ( !bounceHall0.read() ) {
        doorState = 1;
      }
      if ( goState == 1 && slowState == 0 ) {
        doorState = 2;
      }
      if ( bounceHall2.fell() ) {
        doorState = 3;
      }
      if ( slowState == 1 ) {
        doorState = 4;
      }
      if ( !bounceHall3.read() ) {
        doorState = 5;
      }
    }
    
    if ( bounceGO.read() ) {
      dir = LOW;
      digitalWrite(DIR_PIN, dir);
      goSpeed = downGoSpeed;
      slowSpeed = downSlowSpeed;
      
      if ( !bounceHall3.read() ) {
        doorState = 1;
      }
      if ( goState == 1 && slowState == 0 ) {
        doorState = 2;
      }
      if ( bounceHall1.fell() ) {
        doorState = 3;
      }
      if ( slowState == 1 ) {
        doorState = 4;
      }
      if ( !bounceHall0.read() ) {
        doorState = 5;
      }
    }
  }
  else {
    doorState = 5;
  }
  
  switch (doorState) {
    case 1:
      motorAccelerate();
      goState = 1;
      speed = goSpeed;
      break;
    case 2:
      analogWrite(MOTOR_PIN, goSpeed);
      break;
    case 3:
      motorDecelerate();
      speed = slowSpeed;
      slowState = 1;
      break;
    case 4:
      analogWrite(MOTOR_PIN, slowSpeed);
      break;
    case 5:
      analogWrite(MOTOR_PIN, 0);
      goState = 0;
      slowState = 0;
      speed = 0;
      break;
  }

  Serial.print(bounceHall0.read());
  Serial.print('\t');
  Serial.print(bounceHall1.read());
  Serial.print('\t');
  Serial.print(bounceHall2.read());
  Serial.print('\t');
  Serial.print(bounceHall3.read());
  Serial.print('\t');
  Serial.print(bounceOK.read());
  Serial.print('\t');
  Serial.print(bounceGO.read());
  Serial.print('\t');
  Serial.print('\t');
  
  Serial.print(goState);
  Serial.print('\t');
  Serial.print(slowState);
  Serial.print('\t');
  Serial.print(doorState);
  Serial.print('\t');
  Serial.print(speed);
  Serial.print('\t');
  Serial.println();  
}

void motorAccelerate() {
  int pwm;
  int delayTime = 20;
  for(pwm = 0; pwm <= goSpeed; pwm++)
  {
    analogWrite(MOTOR_PIN,pwm);
    delay(delayTime);
  }
}

void motorDecelerate() {
  int pwm;
  int delayTime = 5;
  for(pwm = goSpeed; pwm >= slowSpeed; pwm--)
  {
    analogWrite(MOTOR_PIN,pwm);
    delay(delayTime);
  }
}
