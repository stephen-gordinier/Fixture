#include <SPI.h>
#include <Adafruit_ILI9340.h>
#include <Adafruit_GFX.h>

#define _sclk 13    // Serial Clock
#define _miso 12    // Master-In-Slave-Out
#define _mosi 11    // Master-Out-Slave-In
#define _cs 10      // LCD Chip Select
#define _rst 9      // Reset
#define _dc 8       // Data/Command

// Coordinate arrays for displaying temperatures on the TFT. Change these to change text locations.
// The below "picture" of the TFT is for quick reference for determining index locations.
//  ._____________________.
//  |                     |
//  |  0      1      2    |
//  |  3      4      5    |
//  |  6      7      8    |
//  |                     |
//  |     9      10       |
//  |                     |
//  |     11     12       |
//  |                     |
//  |        13           |
//  |        14           |
//  |_____________________|
//  

int u[15] = {10, 90, 170, 10, 90, 170, 10, 90, 170, 50, 130, 50, 130, 78, 78};
int v[15 = {48, 48, 48, 72, 72, 72, 96, 96, 96, 176, 176, 200, 200, 256, 280};
int labelX[4] = {6, 30, 4, 4};
int labelY[4] = {24, 152, 256, 280};


Adafruit_ILI9340 tft = Adafruit_ILI9340(_cs, _dc, _rst);

uint8_t c = 0;
uint8_t i = 0;

int DOOR_CASE = 1;
int N2_CASE = 1;

void setup() {
  
  Serial.begin(15200);
  
  tftStartup();
  tftLabelLayout();

}

void loop() {
  
  for (c=0; c<13; c++) {
    tft.setCursor(u[c],v[c]);
    tft.print("-00.0");
  }
  if (c==13) {
    doorPrint();
    c++;
  }
  if (c==14) {
    N2Print();
    c++;
  }
  
  if (i == 1) {
    N2_CASE = 1;
    DOOR_CASE = 1;
  }
  if (i == 16) {
    N2_CASE = 2;
    DOOR_CASE = 2;
  }
  if (i == 32) {
    N2_CASE = 3;
    DOOR_CASE = 3;
  }
  if (i == 48) {
    N2_CASE = 4;
    DOOR_CASE = 4;
  }
  if (i == 64) {
    DOOR_CASE = 5;
  }
  if (i == 80) {
    i = 0;
  }
  i++;
}

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
  tft.setCursor(labelX[4],labelY[4]);
  tft.print(" DAQ:");
}

void doorPrint()
{
  tft.setCursor(u[13],v[13]);
  switch(DOOR_CASE) {
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
  tft.setTextColor(WHITE);
}

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