#include <SPI.h>
#include <Wire.h>
#include <WiFi101.h>
#include <SD.h>
#include <Adafruit_INA219.h>

  ////////////////////
 // SETUP FOR WIFI //
////////////////////
#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;

WiFiServer server(80);

  ///////////////////////////
 // SETUP FOR SD LOGGING  //
///////////////////////////
const int CS_PIN = 14; // define card select pin for sd logging
File file;  // create file for sd logging
File nextFile;
float trialNum = 0.0;
String meterFile;

  /////////////////////////////////////
 // VARIABLES FOR ENERGY MONITORING //
/////////////////////////////////////
#define CH3_V_PIN A3  // A3 for Adafruit Feather M0 w/ ATWINC1500, A6 for Arduino Nano
#define CH3_A_PIN A1  // pin A1 for Adafruit Feather M0 w/ ATWINC1500 and Arduino Nano
#define CH4_V_PIN A4  // A4 for Adafruit Feather M0 w/ ATWINC1500, A7 for Arduino Nano
#define CH4_A_PIN A2  // pin A1 for Adafruit Feather M0 w/ ATWINC1500 and Arduino Nano

#define VBATPIN A7  // define BAT pin to measure battery voltage level
float measuredVBat;

int interval = 250; // setting ms interval to calibrate energy calcs
int numCycles = 0;  // records number of interval cycles to space out energy calcs
int logCounter = 0; // records number of energy calcs to space out printing results
int logRate = 4; // sets number of intervals before each print intervals/print

//declare and initialize energy variables for all 4 channels
float energy_Wh_A = 0;
float energy_Wh_B = 0;
float CH3_E = 0;
float CH4_E = 0;

float shuntvoltage_A = 0;
float busvoltage_A = 0;
float current_A_A = 0;
float loadvoltage_A = 0;
float power_W_A = 0;

float shuntvoltage_B = 0;
float busvoltage_B = 0;
float current_A_B = 0;
float loadvoltage_B = 0;
float power_W_B = 0;

float ch3_vsence = 0;
float CH3_V = 0;
float ch3_asence = 0;
float CH3_A = 0;
float CH3_P = 0;

float ch4_vsence = 0;
float CH4_V = 0;
float ch4_asence = 0;
float CH4_A = 0;
float CH4_P = 0;

//  declare variables for average voltage and amperage over logRate
float CH1_V_avg = 0;  // channel 1
float CH1_A_avg = 0;
float CH2_V_avg = 0;  // channel 2
float CH2_A_avg = 0;
float CH3_V_avg = 0;  // channel 3
float CH3_A_avg = 0;
float CH4_V_avg = 0;  // channel 4
float CH4_A_avg = 0;

  /////////////////////////////////////////////////
 // DECLARING PERIPHERIES FOR ENERGY MONITORING //
/////////////////////////////////////////////////
Adafruit_INA219 ina219_A;
Adafruit_INA219 ina219_B(0x41);

void setup()  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
  /*
  // initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("serial connection established"); // for debugging
  */
    /////////////////
   // SD LOGGING  //
  /////////////////
  initializeSD();
  
  Serial.println("test"); // for debugging
  
  // create log file name incremented from last file
  nextFile = SD.open("/");
  nextFile.rewindDirectory();
  while(nextFile)
  {
    trialNum++;
    nextFile = nextFile.openNextFile();
  }
  meterFile = "trial" + String(trialNum-1) + ".txt";
  
  Serial.println("test"); // for debugging
  
  createFile(meterFile);
  closeFile();
  
    ///////////////////////
   // ENERGY MONITORING //
  ///////////////////////
  // start INA boards
  ina219_A.begin();
  ina219_B.begin();
  /*
    ////////////
   //  WIFI  //
  ////////////
  // set wifi to auto sleep
  //WiFi.lowPowerMode();
  // configure pins for Adafruit ATWINC1500 Feather
  WiFi.setPins(8,7,4,2);
  
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD)
  {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  
  server.begin();
  // you're connected now, so print out the status:
  printWiFiStatus();
  */
}

void loop() //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
{
  // read, calculate and assign meter battery voltage
  measuredVBat = analogRead(VBATPIN);
  measuredVBat *= 2;    // double-100K resistor divider on BAT pin, so multiply back
  measuredVBat *= 3.3;  // multiply by 3.3V, the reference voltage
  measuredVBat /= 1024; // convert to voltage
  
  if (millis() >= interval * numCycles)
  {
    // print battery voltage
    Serial.print("VBat: " );
    Serial.println(measuredVBat);
    
    //assign V, A, P and E for 4 channels
    ch3_vsence = analogRead(CH3_V_PIN);
    CH3_V = ch3_vsence * (3.3 / 1023) * 9.8;  //analogRead voltage translated to volts: normalized for logic voltage 3.3V and 8 bit data
    ch3_asence = analogRead(CH3_A_PIN);
    CH3_A = abs((ch3_asence - 512) / 512 * 45); //analogRead amperage translated to amps
    CH3_P = CH3_V * CH3_A;
    CH3_E = CH3_E + CH3_P * (interval/1000) / (3600);
    
    ch4_vsence = analogRead(CH4_V_PIN);
    CH4_V = ch4_vsence * (3.3 / 1023) * 9.8;  //analogRead voltage translated to volts: normalized for logic voltage 3.3V and 8 bit data
    ch4_asence = analogRead(A2);
    CH4_A = abs((ch4_asence - 512) / 512 * 46.5); //analogRead amperage translated to amps
    CH4_P = CH4_V * CH4_A;
    CH4_E = CH4_E + CH4_P * (interval/1000) / (3600);
    
    shuntvoltage_A = ina219_A.getShuntVoltage_mV();
    busvoltage_A = ina219_A.getBusVoltage_V();
    current_A_A = ina219_A.getCurrent_mA() /1000; //assign current (in A i.e. amps) for channel A
    power_W_A = ina219_A.getPower_mW() /1000;
    loadvoltage_A = busvoltage_A + (shuntvoltage_A / 1000); //assign voltage for channel A
    energy_Wh_A = (energy_Wh_A + (power_W_A * (interval/1000) / (3600)));
    
    shuntvoltage_B = ina219_B.getShuntVoltage_mV();
    busvoltage_B = ina219_B.getBusVoltage_V();
    current_A_B = ina219_B.getCurrent_mA() /1000; //assign current (in A i.e. amps) for channel B
    power_W_B = ina219_B.getPower_mW() /1000;
    loadvoltage_B = busvoltage_B + (shuntvoltage_B / 1000); //assign voltage for channel B
    energy_Wh_B = energy_Wh_B + power_W_B * (interval/1000) / (3600);
    
    
    
    // log energy calcs to SD card
    if (logCounter == logRate)
    {
      openFile(meterFile);
      writeToFileLn("time " + String(millis()) + "CH1 " + String(CH1_V_avg) + " " + String(CH1_A_avg));
      writeToFileLn("time " + String(millis()) + "CH2 " + String(CH2_V_avg) + " " + String(CH2_A_avg));
      writeToFileLn("time " + String(millis()) + "CH3 " + String(CH3_V_avg) + " " + String(CH3_A_avg));
      writeToFileLn("time " + String(millis()) + "CH4 " + String(CH4_V_avg) + " " + String(CH4_A_avg));
      closeFile();

      logCounter = 0;
    }
    else  // average the voltage and amperage
    {
      CH1_V_avg = CH1_V_avg*logCounter + loadvoltage_A; // channel 1
      CH1_A_avg = CH1_A_avg*logCounter + current_A_A;
      CH2_V_avg = CH2_V_avg*logCounter + loadvoltage_B; // channel 2
      CH2_A_avg = CH2_A_avg*logCounter + current_A_B;
      CH3_V_avg = CH3_V_avg*logCounter + CH3_V; // channel 3
      CH3_A_avg = CH3_A_avg*logCounter + CH3_A;
      CH4_V_avg = CH3_V_avg*logCounter + CH4_V; // channel 4
      CH4_A_avg = CH3_A_avg*logCounter + CH4_A;

      logCounter++;
      
      CH1_V_avg /= logCounter;  // channel 1
      CH1_A_avg /= logCounter;
      CH2_V_avg /= logCounter;  // channel 2
      CH2_A_avg /= logCounter;
      CH3_V_avg /= logCounter;  // channel 3
      CH3_A_avg /= logCounter;
      CH4_V_avg /= logCounter;  // channel 4
      CH4_A_avg /= logCounter;
    }
    
    numCycles++;
  }
  /*
  Serial.println("checking for available client next"); // debugging
  
  // listen for incoming clients
  WiFiClient client = server.available();
  if (client)
  {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank)
        {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");

          //print current millis
          client.print("sec since init: ");
          client.print(millis()/1000);
          client.println("<br />"); // break line?
          
          client.print("CH3_V: ");
          client.print(CH3_V);
          client.print("; ");
          client.print("CH3_A: ");
          client.print(CH3_A);
          client.print("; ");
          
          client.print("CH4_V: ");
          client.print(CH4_V);
          client.print("; ");
          client.print("CH4_A: ");
          client.print(CH4_A);
          client.print("; ");
          
          client.println("<br />");
          
          // output the battery voltage
          client.print("battery voltage: ");
          client.print(measuredVBat);

          client.println("</html>");
          break;
        }
        if (c == '\n')
        {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r')
        {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    
    // print current millis()
    Serial.print("sec since init: ");
    Serial.println(millis()/1000);
    
    // print battery voltage
    Serial.print("VBat: " );
    Serial.println(measuredVBat);
    
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
    Serial.println("");
  }*/
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  ////////////
 //  WIFI  //
////////////
void printWiFiStatus()
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

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

int writeToFileLn(String text)
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
