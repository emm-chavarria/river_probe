//////// LIBRARIES /////////
#include <Wire.h>
#include <RTClib.h>
#include <math.h>

#include <SD.h>
#define FILE_NAME "Datafile.txt"


//////// GLOBAL VARIABLES //////////

const int ECHO_TO_SERIAL= 1; // echo data to serial port (1=ON, 0=OFF)
const int WAIT_TO_START= 0; // Wait for serial input in setup() (1=ON, 0=OFF)

const int chipSelect = 10; // for the data logging shield, we use digital pin 10 for the SD cs line
const int interval= 3600; // how often to log data (in seconds)

/* TEMPERATURE:
 *  This version utilizes the Steinhart-Hart Thermistor Equation:
 *  Temperature in Kelvin = 1 / {A + B[ln(R)] + C[ln(R)]^3}
 *  for the themistor in the Vernier TMP-BTA probe:
 *  A =0.00102119 , B = 0.000222468 and C = 1.33342E-7
 *  Using these values should get agreement within 1 degree C to the 
 *  same probe used with one of the Vernier interfaces.
 */
 
// Connect temperature probe to BTA1 on Vernier shield
const int ThermistorPIN= 0; // Temperature probe, analog 0
int TempCount;
float Temp;


/* pH:
 *  The pH Sensor produces a voltage of approximately 1.75 volts in a pH 7 buffer.
 *  The voltage increases by about 0.25 volts for every pH number decrease.
 *  The voltage decreases by about 0.25 volts/pH number as the pH increases.
 */
 
// Connect pH probe to BTA2 on Vernier shield
const int phPIN= 2; // pH probe, analog 2
const float phIntercept = 15.0;
const float phSlope = -4.0;


RTC_DS1307 RTC; // Real Time Clock object
File logfile; // the logging file

bool error = false; //Indicates if logging device is ok 


void setup () 
{
    Serial.begin(9600);
    Serial.println("Dual sensor & SD Logger");
    pinMode(LED_BUILTIN, OUTPUT); //Built-in LED
    digitalWrite(LED_BUILTIN, LOW);
    
//    builtInLED();   // Flashes 3 times indicating board is ON
    
    Wire.begin();  
    RTC.begin();
    
    if (!RTC.begin()) {
     logfile.println("RTC failed");
     if (ECHO_TO_SERIAL== 1){
     Serial.println("RTC failed");
     }
   }
   else {
//    RTC.adjust(DateTime(F(__DATE__), F(__TIME__))); // Uncomment to re-adjust time with computer
   }
  
    setupLogFile();
}





void loop () 
{
  DateTime now = RTC.now(); // fetch the time

  if (now.unixtime() % interval == 0){  // Log data this often

    digitalWrite(LED_BUILTIN, HIGH);
    
    TempCount = analogRead(ThermistorPIN);
    Temp = Thermistor(TempCount);
    
    float phCount = analogRead(phPIN);
    float phVoltage = phCount / 1023 * 5.0;// convert from count to raw voltage
    float pH = phIntercept + phVoltage * phSlope; //converts voltage to sensor reading

    
    File logfile = SD.open(FILE_NAME, FILE_WRITE);

    // Print time stamp
    logfile.print(now.day(), DEC);
    logfile.print("/");
    logfile.print(now.month(), DEC);
    logfile.print("/");
    logfile.print(now.year(), DEC);
    logfile.print("\t");
    logfile.print(now.hour(), DEC);
    logfile.print(":");
    logfile.print(now.minute(), DEC);
    logfile.print(":");
    logfile.print(now.second(), DEC);
    logfile.print("\t");
    logfile.print(Temp);
    logfile.print("\t");
    logfile.println(pH);
    logfile.flush();
    logfile.close();


    // Print time stamp in serial
    if (ECHO_TO_SERIAL== 1){
      Serial.print(now.day(), DEC);
      Serial.print("/");
      Serial.print(now.month(), DEC);
      Serial.print("/");
      Serial.print(now.year(), DEC);
      Serial.print(" ");
      Serial.print(now.hour(), DEC);
      Serial.print(":");
      Serial.print(now.minute(), DEC);
      Serial.print(":");
      Serial.print(now.second(), DEC);
      Serial.print("\t");
      Serial.print(Temp);
      Serial.print("\t");
      Serial.println(pH);
    }
    delay(1000);
  }
  else{
    digitalWrite(LED_BUILTIN, LOW); // Turn off blue LED
  }
}


void setupLogFile()
{
  //Initialise the SD card
  while (!SD.begin(chipSelect))
  {
    digitalWrite(LED_BUILTIN, HIGH);   
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    Serial.println("Error: SD card failed to initialise or is missing.");
//    bool error = false;
//    Serial.println(error);
//    return;
  }
  Serial.println("SD card initialized.");

  //Check if the file already exists
  bool oldFile = SD.exists(FILE_NAME);
  
  //Open the file in write mode
  File logfile = SD.open(FILE_NAME, FILE_WRITE);
  
  //Add header information if the file did not already exist
  if (!oldFile)
  {
    logfile.print("Date (dd/mm/yyyy)");
    logfile.print("\t");
    logfile.print("Time (hh:mm:ss)");
    logfile.print("\t");
    logfile.print("Temperature (C)");
    logfile.print("\t");
    logfile.println("pH");
  }
  
  //Close the file to save it
  logfile.flush();
  logfile.close();
}


void builtInLED() // Flashes 3 times indicating board is ON
{
  digitalWrite(LED_BUILTIN, LOW);
  
  for (int i=1; i <= 3; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);   
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
    }
  digitalWrite(LED_BUILTIN, LOW);
}

//void error()
//{
//  for (int i=1; i<=100; i++)
//  {
//    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
//    delay(50);                       // wait for a second
//    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
//    delay(300); // wait for a second
//    }
//}

//Calculates temperature based on resistance readings from probe.
float Thermistor(int Raw)
{ //This function calculates temperature from ADC count
  long Resistance; 
  float Resistor = 15000; //fixed resistor
  // the measured resistance of your particular fixed resistor in
  // the Vernier BTA-ELV and in the SparkFun Vernier Adapter Shield 
  // is a precision 15K resisitor 
  float Temp;  // Dual-Purpose variable to save space.
  Resistance=( Resistor*Raw /(1024-Raw)); 
  Temp = log(Resistance); // Saving the Log(resistance) so not to calculate  it 4 times later
  Temp = 1 / (0.00102119 + (0.000222468 * Temp) + (0.000000133342 * Temp * Temp * Temp));
  Temp = Temp - 273.15;  // Convert Kelvin to Celsius                      
  return Temp;  // Return the Temperature
}
