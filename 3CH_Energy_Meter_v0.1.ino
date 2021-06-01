// 3 channel energy meter with auto file naming
#include <SPI.h>
#include <Wire.h>
#include <WiFi101.h>
#include <SD.h>
#include <Adafruit_INA219.h>

  ///////////////////////////
 // SETUP FOR SD LOGGING  //
///////////////////////////
const int CS_PIN = 19; // define card select pin for sd logging
File file;  // create file for sd logging
int trialNum = 0;

  /////////////////////////////////////
 // VARIABLES FOR ENERGY MONITORING //
/////////////////////////////////////
#define CH2_V_PIN A3  // for Adafruit Feather M0 w/ ATWINC1500, A6 for Arduino Nano
#define CH2_A_PIN A1
#define CH3_V_PIN A4  // for Adafruit Feather M0 w/ ATWINC1500, A7 for Arduino Nano
#define CH3_A_PIN A2

#define VBATPIN A7  // define BAT pin to measure battery voltage level
float measuredVBat;

int interval = 1000; // setting ms interval to calibrate energy calcs
unsigned long prevMillis = 0; //  records time at last recorded energy calcs

//declare and initialize energy variables for all channels
float energy_Wh_A = 0;
float CH2_E = 0;
float CH3_E = 0;

float shuntvoltage_A = 0;
float busvoltage_A = 0;
float current_A_A = 0;
float loadvoltage_A = 0;
float power_W_A = 0;

float ch2_vsence = 0;
float CH2_V = 0;
float ch2_asence = 0;
float CH2_A = 0;
float CH2_P = 0;

float ch3_vsence = 0;
float CH3_V = 0;
float ch3_asence = 0;
float CH3_A = 0;
float CH3_P = 0;

  /////////////////////////////////////////////////
 // DECLARING PERIPHERIES FOR ENERGY MONITORING //
/////////////////////////////////////////////////
Adafruit_INA219 ina219_A(0x41);

void setup()  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
  // initialize serial and wait for port to open:
  Serial.begin(115200);
  /*
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("serial connection established"); // for debugging
  */
  Serial.println("check");
  
    /////////////////
   // SD LOGGING  //
  /////////////////
  initializeSD();
  
  trialNum = 0;
  
  while(SD.exists("trial" + String(trialNum) + ".txt"))
  {
    trialNum++;
    Serial.println(trialNum);
  }
  file = SD.open("trial" + String(trialNum) + ".txt", FILE_WRITE);
  Serial.println(file.name());
  
    ///////////////////////
   // ENERGY MONITORING //
  ///////////////////////
  // start INA boards
  ina219_A.begin();
}

void loop() //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
  // read, calculate and assign meter battery voltage
  measuredVBat = analogRead(VBATPIN);
  measuredVBat *= 2;    // double-100K resistor divider on BAT pin, so multiply back
  measuredVBat *= 3.3;  // multiply by 3.3V, the reference voltage
  measuredVBat /= 1024; // convert to voltage
  
  if (millis() - prevMillis >= interval)
  {
    // print battery voltage
    Serial.print("VBat: " );
    Serial.println(measuredVBat);
    
    //assign V, A, P and E for 4 channels
    ch2_vsence = analogRead(CH2_V_PIN);
    CH2_V = ch2_vsence * (3.3 / 1023) * 9.8;  //analogRead voltage translated to volts: normalized for logic voltage 3.3V and 8 bit data
    ch2_asence = analogRead(CH2_A_PIN);
    CH2_A = abs((ch2_asence - 512) / 512 * 45); //analogRead amperage translated to amps
    CH2_P = CH2_V * CH2_A;
    CH2_E = CH2_E + CH2_P * (interval/1000) / (3600);
    
    ch3_vsence = analogRead(CH3_V_PIN);
    CH3_V = ch3_vsence * (3.3 / 1023) * 9.8;  //analogRead voltage translated to volts: normalized for logic voltage 3.3V and 8 bit data
    ch3_asence = analogRead(A2);
    CH3_A = abs((ch3_asence - 512) / 512 * 46.5); //analogRead amperage translated to amps
    CH3_P = CH3_V * CH3_A;
    CH3_E = CH3_E + CH3_P * (interval/1000) / (3600);
    
    shuntvoltage_A = ina219_A.getShuntVoltage_mV();
    busvoltage_A = ina219_A.getBusVoltage_V();
    current_A_A = ina219_A.getCurrent_mA() /1000; //assign current (in A i.e. amps) for channel A
    power_W_A = ina219_A.getPower_mW() /1000;
    loadvoltage_A = busvoltage_A + (shuntvoltage_A / 1000); //assign voltage for channel A
    energy_Wh_A = (energy_Wh_A + (power_W_A * (interval/1000) / (3600)));
    
    // log energy calcs to SD card
    openFile(file.name());
    writeToFileLn("VBat: " + String(measuredVBat));
    writeToFileLn("time " + String(millis()) + " CH1 " + String(loadvoltage_A) + " " + String(current_A_A));
    writeToFileLn("time " + String(millis()) + " CH2 " + String(CH2_V) + " " + String(CH2_A));
    writeToFileLn("time " + String(millis()) + " CH3 " + String(CH3_V) + " " + String(CH3_A));
    closeFile();
    
    prevMillis = millis();
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  /////////////////
 // SD LOGGING  //
/////////////////
void initializeSD()
{
  Serial.println("initializing SD card...");
  pinMode(CS_PIN, OUTPUT);

  if (SD.begin())
  {
    Serial.println("SD card is ready to use");
  }
  else
  {
    Serial.println("SD card initialization failed");
    return;
  }
}

int createFile(String filename)
{
  file = SD.open(filename, FILE_WRITE);

  if (file)
  {
    //file.print(""); // file must be printed to to be available to open and be written to
    Serial.print("file created successfully: ");
    Serial.println(filename);
    return 1;
  }
  else
  {
    Serial.print("error while creating file: ");
    Serial.println(filename);
    return 0;
  }
}

int writeToFileLn(String text)  // write text to file followed by new line character
{
  if (file)
  {
    file.println(text);
    //Serial.println("writing to file: ");
    Serial.println(text);
    return 1;
  }
  else
  {
    Serial.println("failed to write to file");
    return 0;
  }
}

int writeToFile(String text)
{
  if (file)
  {
    file.print(text);
    //Serial.println("writing to file: ");
    Serial.println(text);
    return 1;
  }
  else
  {
    Serial.println("failed to write to file");
    return 0;
  }
}

void closeFile()
{
  if (file)
  {
    file.close();
    Serial.println("file closed");
  }
}

int openFile(String filename)
{
  file = SD.open(filename, FILE_WRITE);
  
  if (file)
  {
    Serial.print("file opened successfully: ");
    Serial.println(filename);
    return 1;
  }
  else
  {
    Serial.print("error while opening file: ");
    Serial.println(filename);
    return 0;
  }
}

String readLine()
{
  String received = "";
  char ch;
  while (file.available())
  {
    ch = file.read();
    if (ch == '\n')
    {
      return String(received);
    }
    else
    {
      received += ch;
    }
  }
  return "no data";
}

void readFile(String filename)
{
  // open filename for reading:
  file = SD.open(filename);
  if (file)
  {
    Serial.println("reading " + filename + ": ");

    // read from the file until there's nothing else in it:
    while (file.available())
    {
      Serial.write(file.read());
    }
    // close the file:
    file.close();
  }
  else
  {
    // if filename did not open, print an error:
    Serial.println("error opening " + filename);
  }
  Serial.println("done reading file");
  closeFile();
}
