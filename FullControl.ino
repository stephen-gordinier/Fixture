#include <SPI.h>
#include <MuxShield.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9340.h>
#include <Bounce.h>

  // Designations for TFT screen pins
#define _SCLK 13    // Serial Clock
#define _MISO 12    // Master-In-Slave-Out
#define _MOSI 11    // Master-Out-Slave-In
#define _LCDCS 10   // LCD Chip Select
#define _RST 9      // Reset
#define _DC 8       // Data/Command

  // Designations for multiplexer row functions.
#define BUTTON_ROW 1
#define SURFACE_ROW 2
#define AIR_ROW 3

  // Designations for digital switch pin locations on multiplexer.
#define HALL0_PIN 0
#define HALL1_PIN 1
#define HALL2_PIN 2
#define HALL3_PIN 3
#define OK_PIN 4
#define GO_PIN 5
#define PRIME_PIN 6
#define PUSH_PIN 7

  // Designations for motor and valve control pins.
#define MOTOR_PIN 5
#define DIRECTION_PIN A3
#define VALVE_PIN A4

  // Nominal properties of thermistors.
#define R0_AIR 10000      // Resistance of an air thermistor at 25 C
#define R0_SURFACE 2252   // Resistance of a surface thermistor at 25 C
#define R2 10000          // R2 is the other resistor in the voltage divider circuit (10k, in this case).
#define SAMPLESIZE 3      // SAMPLESIZE is the number of consecutive readings of a single sensor that are averaged together for a more stable value. Larger sample sizes slow down the Arduino considerably.

  // This is the temperature above which VALVE PRIME will allow the N2 valve to open.
#define MAX_TEMPERATURE -39.5

  // These lines set up the global temperature reading array.
float T[2][9];
float AVERAGE_TEMPERATURE;
float AIR_QUADRANT_AVGS[4];

  // These are the various state designators for the door and N2 valve.
bool GO_STATE = LOW;
bool SLOW_STATE = LOW;
bool DIRECTION = HIGH;
int GO_SPEED;
int SLOW_SPEED;
int DOOR_CASE;
int SPEED;
int N2_CASE;

  // These are the door's maximum speeds at various stages of opening and closing, out of 255
#define UP_GO_SPEED 150;     // Target speed during opening. Raising above 150 may cause damage to the fixture if the door cannot decelerate quickly enough.
#define UP_SLOW_SPEED 50;    // Slowdown speed during opening.
#define DOWN_GO_SPEED 75;    // Target speed during closing. Not recommended to raise above 100, pending further testing.
#define DOWN_SLOW_SPEED 35;  // Slowdown speed during closing.

  // Coordinate arrays for displaying temperatures on the TFT. Change these to change text locations.
int u[15] = {10, 90, 170, 10, 90, 170, 10, 90, 170, 50, 130, 50, 130, 78, 78};
int v[15] = {48, 48, 48, 72, 72, 72, 96, 96, 96, 176, 176, 200, 200, 256, 280};
int labelX[4] = {6, 30, 4, 4};
int labelY[4] = {24, 152, 256, 280};

  // Initializes instances of various class objects.
MuxShield muxShield;
Adafruit_ILI9340 tft = Adafruit_ILI9340(_LCDCS, _DC, _RST);

  // Initializes an instance of the "debouncer" class for each digital switch. A button press is "debounced" if you want to compensate for noise or jitter in the reading.
Bounce bounceHall0 = Bounce();
Bounce bounceHall1 = Bounce();
Bounce bounceHall2 = Bounce();
Bounce bounceHall3 = Bounce();
Bounce bounceOK = Bounce();
Bounce bounceGO = Bounce();
Bounce bouncePRIME = Bounce();
Bounce bouncePUSH = Bounce();

//--------------------------------------------------------------------------------------------------------//      SETUP      //--------------------------------------------------------------------------------------------------------//
void setup() {

  Serial.begin(115200);
  
  muxShield.setMode(BUTTON_ROW, DIGITAL_IN_PULLUP);
  muxShield.setMode(SURFACE_ROW,ANALOG_IN);
  muxShield.setMode(AIR_ROW,ANALOG_IN);
  
  // Attach each debouncer instance to a pin. This function has been modified in the debouncer class library to work with the multiplexer shield.
  bounceHall0.attach(BUTTON_ROW, HALL0_PIN);
  bounceHall1.attach(BUTTON_ROW, HALL1_PIN);
  bounceHall2.attach(BUTTON_ROW, HALL2_PIN);
  bounceHall3.attach(BUTTON_ROW, HALL3_PIN);
  bounceOK.attach(BUTTON_ROW, OK_PIN);
  bounceGO.attach(BUTTON_ROW, GO_PIN);
  bouncePRIME.attach(BUTTON_ROW, PRIME_PIN);
  bouncePUSH.attach(BUTTON_ROW, PUSH_PIN);

  // Set the debouncing interval for each instance.
  bounceHall0.interval(50);
  bounceHall1.interval(50);
  bounceHall2.interval(10);  // This instance
  bounceHall3.interval(10);  // and this one represent the upper two Hall effect sensors. They should have a higher refresh rate, to compensate for the door moving quickly.
  bounceOK.interval(50);
  bounceGO.interval(50);
  bouncePRIME.interval(50);
  bouncePUSH.interval(50);

  pinMode(MOTOR_PIN,OUTPUT);
  pinMode(DIRECTION_PIN,OUTPUT);
  analogWrite(MOTOR_PIN,0);
  analogWrite(DIRECTION_PIN,0);
  
  tftStartup();
  tftLabelLayout();
  
}

//--------------------------------------------------------------------------------------------------------//      LOOP      //--------------------------------------------------------------------------------------------------------//

void loop() {
  
  // Update the readings of all digital sensors and switches
  bounceHall0.update();
  bounceHall1.update();
  bounceHall2.update();
  bounceHall3.update();
  bounceOK.update();
  bounceGO.update();
  bouncePRIME.update();
  bouncePUSH.update();
    
  // Execute door control logic
  if ( !bounceOK.read() ) {       // If the DOOR ENABLE switch is enabled
    doorStateCheck();             // Check the door's status and update its case value
  } else {                        // Otherwise
    DOOR_CASE = 5;                // Set the door's case value to 5 (full stop.)
  }
  doorMotion();                   // Execute motor commands based on the door case.
  
  // Read all thermistors and update the temperature reading array
  tempRead();
  
  // Execute N2 control logic
  N2Control();

  // Updates all information on the TFT screen.
  tempPrint();
  doorPrint();
  N2Print();

}

//--------------------------------------------------------------------------------------------------------// SETUP FUNCTIONS //--------------------------------------------------------------------------------------------------------//

//-------------------------------------------------// TFT STARTUP
void tftStartup()
{
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(BLUE);
  tft.setCursor(0,64);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.print("STARTUP...");
  delay(500);
}

//-------------------------------------------------// TFT LABEL LAYOUT
void tftLabelLayout()
{
  uint8_t i;
  tft.fillScreen(BLUE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(labelX[0],labelY[0]);
  tft.print("SURFACE THERMISTORS");
  tft.setCursor(labelX[1],labelY[1]);
  tft.print("AIR THERMISTORS");
  tft.setCursor(labelX[2],labelY[2]);
  tft.print("DOOR:");
  tft.setCursor(labelX[3],labelY[3]);
  tft.print("  N2:");
}

//--------------------------------------------------------------------------------------------------------// LOOP FUNCTIONS //-------------------------------------------------------------------------------------------------------//

//-------------------------------------------------// DOOR STATE CHECK
void doorStateCheck()
{
  digitalWrite(DIRECTION_PIN, DIRECTION);          // Set the motor direction.
  
  if ( !bounceGO.read() ) {                        // If the DOOR GO switch is flipped upward
    DIRECTION = HIGH;                              // Set the direction
    digitalWrite(DIRECTION_PIN, DIRECTION);        // Write the direction immediately
    GO_SPEED = UP_GO_SPEED;                        // Set the movement speed max
    SLOW_SPEED = UP_SLOW_SPEED;                    // Set the slowdown speed
    
    if ( !bounceHall0.read() ) {                   // If the bottom Hall effect sensor is tripped
      DOOR_CASE = 1;                               // The door is in state 1: accelerate
    }
    if ( GO_STATE == 1 && SLOW_STATE == 0 ) {      // Once the door has accelerated upward, proceed
      DOOR_CASE = 2;                               // The door is in state 2: in motion at top speed
    }
    if ( bounceHall2.fell() ) {                    // If the second-from-the-top Hall effect sensor is tripped
      DOOR_CASE = 3;                               // The door is in state 3: decelerate
    }
    if ( SLOW_STATE == 1 ) {                       // Once the door has decelerated, proceed
      DOOR_CASE = 4;                               // The door is in state 4: going slowly, about to stop
    }
    if ( !bounceHall3.read() ) {                   // If the top Hall effect sensor is active,
      DOOR_CASE = 5;                               // The door is in state 5: at rest, in position
    }
  }
  if ( bounceGO.read() ) {                         // This is the same loop as above, except reversed, for lowering the door.
    DIRECTION = LOW;
    digitalWrite(DIRECTION_PIN, DIRECTION);
    GO_SPEED = DOWN_GO_SPEED;
    SLOW_SPEED = DOWN_SLOW_SPEED;
    
    if ( !bounceHall3.read() ) {
      DOOR_CASE = 1;
    }
    if ( GO_STATE == 1 && SLOW_STATE == 0 ) {
      DOOR_CASE = 2;
    }
    if ( bounceHall1.fell() ) {
      DOOR_CASE = 3;
    }
    if ( SLOW_STATE == 1 ) {
      DOOR_CASE = 4;
    }
    if ( !bounceHall0.read() ) {
      DOOR_CASE = 5;
    }
  }
}

//-------------------------------------------------// DOOR MOTION
void doorMotion()
{
  switch (DOOR_CASE) {                             // This function determines what actually occurs, given the door's state.
  case 1:                                          // Accelerating
    motorAccelerate();
    GO_STATE = 1;
    SPEED = GO_SPEED;
    break;
  case 2:                                          // In motion, at full speed
    analogWrite(MOTOR_PIN, GO_SPEED);
    break;
  case 3:                                          // Decelerating
    motorDecelerate();
    SLOW_STATE = 1;
    SPEED = SLOW_SPEED;
    break;
  case 4:                                          // Going slow
    analogWrite(MOTOR_PIN, SLOW_SPEED);
    break;
  case 5:                                          // At rest, in position
    analogWrite(MOTOR_PIN, 0);
    GO_STATE = 0;
    SLOW_STATE = 0;
    SPEED = 0;
    break;
  }
}

//-------------------------------------------------// MOTOR ACCELERATE
void motorAccelerate()
{
  int pwm;
  int delayTime = 20;
  for(pwm = 0; pwm <= GO_SPEED; pwm++)
  {
    analogWrite(MOTOR_PIN,pwm);
    delay(delayTime);
  }
}

//-------------------------------------------------// MOTOR DECELERATE
void motorDecelerate()
{
  int pwm;
  int delayTime = 5;
  for(pwm = GO_SPEED; pwm >= SLOW_SPEED; pwm--)
  {
    analogWrite(MOTOR_PIN,pwm);
    delay(delayTime);
  }
}

//-------------------------------------------------// TEMPERATURE READ
void tempRead()
{                                                  //  This function builds the temperature reading array T.
  uint8_t i;                                       //  The array is a 2-row, 9-column matrix. Row 0 is surface thermistors, row 1 is air thermistors.
  uint8_t n;
  float R[9];
  float reading[9];
  int sum;
  
  for (i=0; i<9; i++) {
    sum = 0;
    for (n=0; n<SAMPLESIZE; n++) {
      sum += muxShield.analogReadMS(SURFACE_ROW,i);
      delay(1);
    }
    reading[i] = sum/SAMPLESIZE;
    T[0][i] = Steinhart(reading[i], R0_SURFACE, R2);  // Calls the "Steinhart" conversion formula using the averaged analog reading (0-1023), the nominal resistance of the thermistor, and R2.
    sum = 0;
    for (n=0; n<SAMPLESIZE; n++) {
      sum += muxShield.analogReadMS(AIR_ROW,i);
      delay(1);
    }
    reading[i] = sum/SAMPLESIZE;
    T[1][i] = Steinhart(reading[i], R0_AIR, R2);      // Calls the "Steinhart" conversion formula again, for air probes.
  }
}

//-------------------------------------------------// STEINHART-HART CONVERSION FORMULA
float Steinhart(float res, int res0, int res2)
{                                                  // This function converts a thermistor resistance to a temperature in degrees Celsius.
    float temp;
    temp = (1/((1/298.15) + (log((res2/((1023/res) - 1))/res0))/3950)) - 273.15;
    return temp;
}

//-------------------------------------------------// N2 CONTROL
void N2Control()
{                                                  // This function handles nitrogen valve control logic.
  if ( !bouncePRIME.read() && !bounceHall0.read() && bounceOK.read() ) {    // If N2 PRIME is enabled, the door is CLOSED, and DOOR ENABLE is not active,
      averageTemperature();                                                 // Check the average temperature of the surface thermistors.
      if ( AVERAGE_TEMPERATURE >= MAX_TEMPERATURE ) {                       // If the average temperature is greater than or equal to the maximum temperature,
        digitalWrite(VALVE_PIN, HIGH);                                      // Open the valve.
        N2_CASE = 3;                                                       // Valve state is PRIME, OPENED
      } else if ( !bouncePUSH.read() ) {                                    // If N2 PUSH is enabled,
        digitalWrite(VALVE_PIN, HIGH);                                      // Open the valve.
        N2_CASE = 4;                                                       // Valve state is --PUSH OPEN--
      } else {                                                              // Otherwise,
        digitalWrite(VALVE_PIN, LOW);                                       // Close the valve.
        N2_CASE = 2;                                                       // Valve state is PRIME, CLOSED
      }
  } else {                                                                  // If all required conditions are not met,
    digitalWrite(VALVE_PIN, LOW);                                           // Close the valve.
    N2_CASE = 1;                                                           // Valve state is SAFE
  }
}

//-------------------------------------------------// AVERAGE TEMPERATURE
void averageTemperature()
{                                                  // This function quickly averages the 9 surface thermistors and updates the global variable.
  uint8_t i;
  for (i=0; i<9; i++) {
    AVERAGE_TEMPERATURE += T[0][i];
  }
  AVERAGE_TEMPERATURE = AVERAGE_TEMPERATURE/9;
}

//-------------------------------------------------// PRINT TEMPERATURES
void tempPrint()
{
  uint8_t i;

  tft.setTextColor(WHITE, BLUE);
  tft.setTextSize(2);

  for (i=0; i<9; i++) {
    tft.setCursor(u[i],v[i]);
    tft.print(T[0][i],1);
  }
  
  airQuadrants();
  
  for (i=9; i<13; i++) {
    tft.setCursor(u[i],v[i]);
    tft.print(AIR_QUADRANT_AVGS[i-9],1);
  }
}

//-------------------------------------------------// AIR QUADRANT AVERAGES
void airQuadrants()
{
  AIR_QUADRANT_AVGS[0] = (T[1][0] + T[1][1] + T[1][3] + T[1][4])/4;
  AIR_QUADRANT_AVGS[1] = (T[1][1] + T[1][2] + T[1][4] + T[1][5])/4;
  AIR_QUADRANT_AVGS[2] = (T[1][3] + T[1][4] + T[1][6] + T[1][7])/4;
  AIR_QUADRANT_AVGS[3] = (T[1][4] + T[1][5] + T[1][7] + T[1][8])/4;  
}

//-------------------------------------------------// PRINT DOOR STATUS
void doorPrint()
{
  int doorLabelCase;
  if ( !bounceOK.read()) {                                                    // DOOR_OK is enabled
    if (DIRECTION == LOW && DOOR_CASE == 5) {                                 // Direction is DOWN, and the door's case is 5
      doorLabelCase = 1;                                                        // The door is CLOSED
    } else if (DIRECTION == HIGH && bounceHall0.read()) {                     // The direction is UP and Hall0 is not tripped
      doorLabelCase = 2;                                                        // The door is OPENING
    } else if (DIRECTION == HIGH && DOOR_CASE == 5) {                         // Direction is UP, and the door's case is 5
      doorLabelCase = 3;                                                        // The door is OPENED
    } else if (DIRECTION == LOW && bounceHall0.read()) {                      // The direction is DOWN and Hall0 is not tripped
      doorLabelCase = 4;                                                        // The door is CLOSING
    }
  } else if (bounceOK.read() && !bounceHall0.read()) {                        // DOOR_OK is disabled, and Hall0 is tripped
    doorLabelCase = 1;                                                          // The door is CLOSED
  } else {                                                                    // DOOR_OK is disabled, and Hall0 is NOT tripped
    doorLabelCase = 5;                                                          // The door is STOPPED
  }
  
  tft.setCursor(u[13],v[13]);
  switch(doorLabelCase) {
    case 1:
      tft.setTextColor(GREEN, BLUE);
      tft.print("CLOSED     ");
      break;
    case 2:
      tft.setTextColor(YELLOW, BLUE);
      tft.print("OPENING    ");
      break;
    case 3:
      tft.setTextColor(GREEN, BLUE);
      tft.print("OPENED     ");
      break;
    case 4:
      tft.setTextColor(YELLOW, BLUE);
      tft.print("CLOSING    ");
      break;
    case 5:
      tft.setTextColor(RED, BLUE);
      tft.print("--STOPPED--");
  }
}

//-------------------------------------------------// PRINT N2 STATUS
void N2Print()
{
  tft.setCursor(u[14],v[14]);
  switch(N2_CASE) {
    case 1:
      tft.setTextColor(GREEN,BLUE);
      tft.print("SAFE         ");
      break;
    case 2:
      tft.setTextColor(YELLOW,BLUE);
      tft.print("PRIME, CLOSED");
      break;
    case 3:
      tft.setTextColor(YELLOW,BLUE);
      tft.print("PRIME, OPEN  ");
      break;
    case 4:
      tft.setTextColor(RED,BLUE);
      tft.print("--PUSH OPEN--");
  }
  tft.setTextColor(WHITE);
}
