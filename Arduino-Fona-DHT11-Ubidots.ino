#include <HardwareSerial.h>
#include "Adafruit_FONA.h"
#include "DHT.h"
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// DHT

#define DHTPIN 4

// Uncomment whatever type you're using!

#define DHTTYPE DHT11   // DHT 11 
//#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);

// Fona

#define FONA_RX 11
#define FONA_TX 12
#define FONA_RST 10
#define FONA_KEY 9
#define FONA_PS 8

// #define HWSERIAL Serial1


char replybuffer[255];

// HardwareSerial fonaSS = Serial1;
// Adafruit_FONA fona = Adafruit_FONA(&fonaSS, FONA_RST);

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
Adafruit_FONA fona = Adafruit_FONA(&fonaSS, FONA_RST);

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);


int Interval = 20000;             // Time between measurements in seconds
int KeyTime = 2000;               // Time needed to turn on the Fona
// unsigned long Reporting = 30000;  // Time between uploads to Ubidots
unsigned long TimeOut = 10000;    // How long we will give an AT command to comeplete
// unsigned long LastReading = 0;    // When did we last read the sensors - they are slow so will read between sends
// unsigned long LastReporting = 0;  // When did we last send data to Ubidots
uint8_t n=0;
// int f = 0;
// int PersonCount = 0;

void setup() {
  delay(3000);
  // HWSERIAL.begin(4800);
  Serial.begin(9600);
  
  dht.begin();

  Serial.println("Started setup");

  // pinMode(FONA_KEY,OUTPUT);                                    // This is the pin used to turn on and off the Fona
  // TurnOnFona();
  Serial.println(F("FONA basic test"));
  Serial.println(F("Initializing....(May take 3 seconds)"));   // See if the FONA is responding
  if (! fona.begin(4800)) {                                    // make it slow so its easy to read!
    Serial.println(F("Couldn't find FONA"));
    while (1);
  }
  GetConnected();
  Serial.println(F("FONA is OK"));
  //TurnOffFona();

}


void loop() {
  Serial.print("In Loop: ");
  Serial.println(millis()/1000);
  delay(10000);
  
  // Leer temperatura
  float temp = dht.readTemperature();
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(temp)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  
  // if (LastReporting + Reporting <= millis()) {   // This checks to see if it is time to send to Ubidots
    // TurnOnFona();                                // Turn on the module
    // GetConnected();                              // Connect to network and start GPRS
    Send2ubidots(String(temp));                 // Send data to Ubidots
    // GetDisconnected();                           // Disconnect from GPRS
    // TurnOffFona();                               // Turn off the modeule
    // LastReporting = millis();
  // }

  /* if (LastReading + Interval <= millis()) {     // This checks to see if it is time to take a sensor reading
    f ++;                 //Do some math to convert the Celsius to Fahrenheit
    PersonCount ++;
    Serial.print(F("The current Temperature is: "));
    Serial.print(f);                             //Send the temperature in degrees F to the serial monitor
    Serial.print(F("F "));
    LastReading = millis();                     // Record the time of the last sensor readings
  }
  */


}

// This function is to send the sensor data to Ubidots - each step is commented in the serial terminal
void Send2ubidots(String value)
{

  int num;
  String le;
  String var;
  var = "{\"value\":"+ value + "}";           //value is the sensor value
  num = var.length();
  le = String(num);                           //this is to calcule the length of var

  Serial.print(F("Start the connection to Ubidots: "));
  if (SendATCommand("AT+CIPSTART=\"TCP\",\"things.ubidots.com\",\"80\"",'C','T')) {
    Serial.println("Connected");
  }
  Serial.print(F("Begin to send data to the remote server: "));
  if (SendATCommand("AT+CIPSEND",'\n','>')) {
    Serial.println("Sending");
  }
 fona.println("POST /api/v1.6/variables/54dcde5876254240b3af12a8/values HTTP/1.1");           // Replace "xxx..." with your variable ID
 fona.println("Content-Type: application/json");
 fona.println("Content-Length: "+le);
 fona.println("X-Auth-Token: ---");                                    // here you should replace "xxx..." with your Ubidots Token
 fona.println("Host: things.ubidots.com");
 fona.println();
 fona.println(var);
 fona.println();
 fona.println((char)26);                                       //This ends the JSON SEND with a carriage return
 Serial.print(F("Send JSON Package: "));
 if (SendATCommand("",'2','0')) {                              // The 200 code from Ubidots means it was successfully uploaded
    Serial.println("Sent");
     // PersonCount = 0;
  }
  else {
    Serial.println("Send Timed out, will retry at next interval");
  }
 // delay(2000);
 Serial.print(F("Close connection to Ubidots: "));              // Close the connection
 if (SendATCommand("AT+CIPCLOSE",'G','M')) {
    Serial.println("Closed");
  }
}

boolean SendATCommand(char Command[], char Value1, char Value2) {
 Serial.println("*********************************");
 Serial.println("Funcion SendATCommand:");
 Serial.print("Comando: ");
 Serial.println(Command);
 unsigned char buffer[64];                                  // buffer array for data recieve over serial port
 int count = 0;
 int complete = 0;
 unsigned long commandClock = millis();                      // Start the timeout clock
 fona.println(Command);
 while(!complete && commandClock <= millis()+TimeOut)         // Need to give the modem time to complete command
 {
   while(!fona.available() && commandClock <= millis()+TimeOut);
   while(fona.available()) {                                 // reading data into char array
     buffer[count++]=fona.read();                            // writing data into array
     if(count == 64) break;
   }
   Serial.write(buffer,count);                           // Uncomment if needed to debug
   for (int i=0; i <= count; i++) {
     if(buffer[i]==Value1 && buffer[i+1]==Value2) complete = 1;
   }
 }
 if (complete ==1) return 1;                              // Returns "True"  - "False" sticks in the loop for now
 else return 0;
}

void TurnOnFona()
{
  Serial.print("Turning on Fona: ");
  while(digitalRead(FONA_PS))
  {
    digitalWrite(FONA_KEY,LOW);
    unsigned long KeyPress = millis();
    while(KeyPress + KeyTime >= millis()) {}
    digitalWrite(FONA_KEY,HIGH);
  }
  fona.begin(4800);
  Serial.println("success!");
}

void GetConnected()
{
  do
  {
    n = fona.getNetworkStatus();  // Read the Network / Cellular Status
    Serial.print(F("Network status "));
    Serial.print(n);
    Serial.print(F(": "));
      if (n == 0) Serial.println(F("Not registered"));
      if (n == 1) Serial.println(F("Registered (home)"));
      if (n == 2) Serial.println(F("Not registered (searching)"));
      if (n == 3) Serial.println(F("Denied"));
      if (n == 4) Serial.println(F("Unknown"));
      if (n == 5) Serial.println(F("Registered roaming"));
  } while (n != 1);
}

void GetDisconnected()
{
  Serial.println("Fona enableGPRS false");
  fona.enableGPRS(false);
  Serial.println(F("GPRS Serivces Started"));
}

void TurnOffFona()
{
  Serial.print("Turning off Fona: ");
  while(digitalRead(FONA_PS))
  {
    digitalWrite(FONA_KEY,LOW);
    unsigned long KeyPress = millis();
    while(KeyPress + KeyTime >= millis()) {}
    digitalWrite(FONA_KEY,HIGH);
  }
  Serial.println("success!");
}
