#include <Bounce.h>
#include <MuxShield.h>

#define BUTTON_ROW 1
#define SURFACE_ROW 2
#define AIR_ROW 3
#define HALL0_PIN 0
#define HALL1_PIN 1
#define HALL2_PIN 2
#define HALL3_PIN 3
#define OK_PIN 4
#define GO_PIN 5
#define PRIME_PIN 6
#define PUSH_PIN 7
#define DAQ_PIN 8
#define MOTOR_PIN 5
#define DIR_PIN A3
#define VALVE_PIN A4

#define R0_AIR 10000
#define R0_SURFACE 2252
#define R2 10000
#define SAMPLESIZE 3

MuxShield muxShield;

Bounce bounceHall0 = Bounce();
Bounce bounceHall1 = Bounce();
Bounce bounceHall2 = Bounce();
Bounce bounceHall3 = Bounce();
Bounce bounceOK = Bounce();
Bounce bounceGO = Bounce();
Bounce bouncePRIME = Bounce();
Bounce bouncePUSH = Bounce();
Bounce bounceDAQ = Bounce();

void setup() {
  muxShield.setMode(BUTTON_ROW, DIGITAL_IN_PULLUP);
  muxShield.setMode(SURFACE_ROW,ANALOG_IN);
  muxShield.setMode(AIR_ROW,ANALOG_IN);
  
  bounceHall0.attach(BUTTON_ROW, HALL0_PIN);
  bounceHall1.attach(BUTTON_ROW, HALL1_PIN);
  bounceHall2.attach(BUTTON_ROW, HALL2_PIN);
  bounceHall3.attach(BUTTON_ROW, HALL3_PIN);
  bounceOK.attach(BUTTON_ROW, OK_PIN);
  bounceGO.attach(BUTTON_ROW, GO_PIN);
  bouncePRIME.attach(BUTTON_ROW, PRIME_PIN);
  bouncePUSH.attach(BUTTON_ROW, PUSH_PIN);
  bounceDAQ.attach(BUTTON_ROW, DAQ_PIN);

  bounceHall0.interval(50);
  bounceHall1.interval(50);
  bounceHall2.interval(10);
  bounceHall3.interval(10);
  bounceOK.interval(50);
  bounceGO.interval(50);
  bouncePRIME.interval(50);
  bouncePUSH.interval(50);
  bounceDAQ.interval(50);

  Serial.begin(115200);

}

void loop() {
  
  // Updates the readings of all digital sensors and switches
  bounceHall0.update();
  bounceHall1.update();
  bounceHall2.update();
  bounceHall3.update();
  bounceOK.update();
  bounceGO.update();
  bouncePRIME.update();
  bouncePUSH.update();
  bounceDAQ.update();

  //  Sets up arrays for thermistor readings, clears memory space
  uint8_t i;
  uint8_t n;
  uint8_t r;
  float T[2][9];
  float R[9];
  float reading[9];
  int sum;
  memset(T,0,sizeof(T));
  memset(R,0,sizeof(R));
  memset(reading,0,sizeof(reading));
  
  //  Builds thermistor reading array, using Steinhart conversion formula
  for (i=0; i<9; i++) {
    sum = 0;
    for (n=0; n<SAMPLESIZE; n++) {
      sum += muxShield.analogReadMS(SURFACE_ROW,i);
      delay(1);
    }
    reading[i] = sum/SAMPLESIZE;
    T[0][i] = Steinhart(reading[i], R0_SURFACE, R2);
    sum = 0;
    for (n=0; n<SAMPLESIZE; n++) {
      sum += muxShield.analogReadMS(AIR_ROW,i);
      delay(1);
    }
    reading[i] = sum/SAMPLESIZE;
    T[1][i] = Steinhart(reading[i], R0_AIR, R2);
  }
  
  //  Prints all switch readings
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
  Serial.print(bouncePRIME.read());
  Serial.print('\t');
  Serial.print(bouncePUSH.read());
  Serial.print('\t');
  Serial.print(bounceDAQ.read());
  Serial.print('\t');
  
  // Prints resistance of each thermistor to serial monitor
//  for (n=0; n<9; n++) {
//    Serial.print(Resistance(muxShield.analogReadMS(3,n), R0_AIR, R2));
//    Serial.print('\t');
//  }
  
  //  Prints temperature values to serial monitor
  for (i=0; i<2; i++) {
    for (n=0; n<9; n++) {
      Serial.print(T[i][n]);
      Serial.print('\t');
    }
  }
  
  Serial.println();

}

float Steinhart(float res, int res0, int res2) {
    float temp;  
    temp = (1/((1/298.15) + (log((res2/((1023/res) - 1))/res0))/3950)) - 273.15;
    return temp;
}

float Resistance(float val, int res0, int res2) {
    float res;  
    res = res2/((1023/val) - 1);
    return res;
}
