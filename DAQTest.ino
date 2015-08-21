#include <SPI.h>
#include <Adafruit_ILI9340.h>
#include <Adafruit_GFX.h>
#include <MuxShield.h>
#include <Bounce.h>
#include <SdFat.h>

  // Designations for TFT screen pins
#define _SCLK 13    // Serial Clock
#define _MISO 12    // Master-In-Slave-Out
#define _MOSI 11    // Master-Out-Slave-In
#define _LCDCS 10   // LCD Chip Select
#define _RST 9      // Reset
#define _DC 8       // Data/Command
#define _SDCS 3     // SD Chip Select

  // Designations for multiplexer row functions.
#define BUTTON_ROW 1
#define SURFACE_ROW 2
#define AIR_ROW 3

  // Designations for digital switch pin locations on multiplexer.
#define DAQ_PIN 8

  // Nominal properties of thermistors.
#define R0_AIR 10000      // Resistance of an air thermistor at 25 C
#define R0_SURFACE 2252   // Resistance of a surface thermistor at 25 C
#define R2 10000          // R2 is the other resistor in the voltage divider circuit (10k, in this case).
#define SAMPLESIZE 3      // SAMPLESIZE is the number of consecutive readings of a single sensor that are averaged together for a more stable value. Larger sample sizes slow down the Arduino considerably.

// Miscellaneous variables that must be defined globally (accessed by multiple functions)
int loopCount;    // Loop counter for the DAQ functions
int FILE_COUNT;    // Counts number of files on the SD card
char *FILE_NAME;   // Name of new DAQ file
float DAQ_START_TIME;  // DAQ timer

int u[15] = {10, 90, 170, 10, 90, 170, 10, 90, 170, 50, 130, 50, 130, 100, 100};
int v[15] = {32, 32, 32, 48, 48, 48, 64, 64, 64, 96, 96, 112, 112, 144, 160};

File daqFile;
SdFat SD;
SdFile file;
MuxShield muxShield;
Adafruit_ILI9340 tft = Adafruit_ILI9340(_LCDCS, _DC, _RST);
Bounce bounceDAQ = Bounce();

void setup()
{
  Serial.begin(115200);

  muxShield.setMode(BUTTON_ROW, DIGITAL_IN_PULLUP);
  muxShield.setMode(SURFACE_ROW,ANALOG_IN);
  muxShield.setMode(AIR_ROW,ANALOG_IN);
  
  bounceDAQ.attach(BUTTON_ROW, DAQ_PIN);
  bounceDAQ.interval(50);

  tftStartup();
  sdStartup();
  tftLabelLayout();
  
}

void loop()
{
  bounceDAQ.update();

  if ( bounceDAQ.fell() ) {
  daqStartup();
  }
  
  tempRead();
  tempPrint();
  
  uint8_t x;
  uint8_t y;
    
  float runTime = millis() - DAQ_START_TIME;
  float time = runTime/1000;
  String dataString = "";
  dataString += String(time);
    
  for (x=0; x<3; x++) {
    tft.setCursor(u[x],v[0]);
    tft.setTextColor(WHITE, DARKBLUE);
    tft.print(T[0][x],1);
    dataString += '\t';
    dataString += T[0][x];
  }
  for (x=3; x<6; x++) {
    tft.setCursor(u[x-3],v[1]);
    tft.setTextColor(WHITE, DARKBLUE);
    tft.print(T[0][x],1);
    dataString += '\t';
    dataString += T[0][x];
  }
  for (x=6; x<9; x++) {
    tft.setCursor(u[x-6],v[2]);
    tft.setTextColor(WHITE, DARKBLUE);
    tft.print(T[0][x],1);
    dataString += '\t';
    dataString += T[0][x];
  }
  for (x=0; x<3; x++) {
    tft.setCursor(u[x],v[3]);
    tft.setTextColor(WHITE, DARKBLUE);
    tft.print(T[1][x],1);
    dataString += '\t';
    dataString += T[1][x];
  }
  for (x=3; x<6; x++) {
    tft.setCursor(u[x-3],v[4]);
    tft.setTextColor(WHITE, DARKBLUE);
    tft.print(T[1][x],1);
    dataString += '\t';
    dataString += T[1][x];
  }
  for (x=6; x<9; x++) {
    tft.setCursor(u[x-6],v[5]);
    tft.setTextColor(WHITE, DARKBLUE);
    tft.print(T[1][x],1);
    dataString += '\t';
    dataString += T[1][x];
  }

  daqFile.println(dataString);
  loopCount++;
  
  if ( bounceDAQ.rose() ) {
    tft.fillScreen(BLUE);
    tft.setCursor(0,0);
    tft.println("Complete!                        ");
    daqFile.close();
    tft.println("File closed.                     ");
    loopCount = 0;
  }

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

void sdStartup()
{
  tft.fillScreen(BLUE);
  tft.setCursor(0,64);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.print("Initializing SD.");
  if (!SD.begin(_SDCS)) {
    tft.setTextColor(RED);
    tft.println();
    tft.println("Card failed, or not present.");
    tft.println("Eject, re-insert, and reset.");
    while(true);
  }
  delay(750);
  tft.print(".");
  delay(750);
  tft.println(".");
  delay(1000);
  tft.setTextColor(GREEN);
  tft.println();
  tft.println("SD card initialized.");
  delay(2000);
  
}

void daqStartup()
{
  FILE_COUNT = fileCounter();
  String nameString = "DAQ"; nameString += FILE_COUNT; nameString += ".txt";
  int nameLength = nameString.length() + 1;
  char nameChar[nameLength];
  nameString.toCharArray(nameChar, 12);
  FILE_NAME = nameChar;
  daqFile = SD.open(FILE_NAME, FILE_WRITE);
  DAQ_START_TIME = millis();
  daqFile.println("Time(s)\t S1\t S2\t S3\t S4\t S5\t S6\t S7\t S8\t S9\t A1\t A2\t A3\t A4\t A5\t A6\t A7\t A8\t A9");
}

int fileCounter()
{
  int k = 0;
  while(file.openNext(SD.vwd(),O_READ)) {
    k++;
    file.close();
  }
  return k;
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

void tempRead()
//  This function builds the temperature reading array T.
//  The array is a 2-row, 9-column matrix. Row 0 is surface thermistors, row 1 is air thermistors.
{
  uint8_t i;
  uint8_t n;
  uint8_t r;
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
    T[1][i] = Steinhart(reading[i], R0_AIR, R2);  // Calls the "Steinhart" conversion formula again, for air probes.
  }
}

float Steinhart(float res, int res0, int res2)
// This function uses the Steinhart-Hart conversion formula to convert a thermistor resistance to a temperature in degrees Celsius.
{
    float temp;  
    temp = (1/((1/298.15) + (log((res2/((1023/res) - 1))/res0))/3950)) - 273.15;
    return temp;
}
