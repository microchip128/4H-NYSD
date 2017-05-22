/*
   Jeff's notes= not working because of Char conversion, need to figure our casting new char?

*/
//#include <SoftwareSerial.h>

/*
  Optical Heart Rate Detection (PBA Algorithm) using the MAX30105 Breakout
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 2nd, 2016
  https://github.com/sparkfun/MAX30105_Breakout

  This is a demo to show the reading of heart rate or beats per minute (BPM) using
  a Penpheral Beat Amplitude (PBA) algorithm.

  It is best to attach the sensor to your finger using a rubber band or other tightening
  device. Humans are generally bad at applying constant pressure to a thing. When you
  press your finger against the sensor it varies enough to cause the blood in your
  finger to flow differently which causes the sensor readings to go wonky.

  Hardware Connections (Breakoutboard to Arduino):
  -5V = 5V (3.3V is allowed)
  -GND = GND
  -SDA = A4 (or SDA)
  -SCL = A5 (or SCL)
  -INT = Not connected

  The MAX30105 Breakout can handle 5V or 3.3V I2C logic. We recommend powering the board with 5V
  but it will also run at 3.3V.
*/

#include <Wire.h>
#include "MAX30105.h"

#include "heartRate.h"

MAX30105 particleSensor;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;
int steps;
int counter = 0;
float temperature;
float temperatureF;
boolean state = 0;
unsigned long last_interrupt=0;
/* Create a WiFi access point and provide a web server on it. */

#include <ESP8266WiFi.h>
//#include <SparkFunESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

/* Set these to your desired credentials. */
char ssid [30];

//const char *password = "thereisnospoon";
String htmlText = ("<!DOCTYPE html><html><head><title>Incrediable Wearables</title></head><body style=background-color:#61c250>");

ESP8266WebServer server(80);

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
   connected to this access point to see it.
*/
void handleRoot() {


  htmlText.concat("<h1>You are connected to Incredible Wearables!</h1>");
  htmlText.concat("<br> <h2>Your heart rate is: ");
  htmlText.concat(String (int (beatAvg)));
  htmlText.concat(" </h2> ");
  htmlText.concat("<br>");
  htmlText.concat("<h2>Your temperature is:  ");
  htmlText.concat(String (temperatureF));
  htmlText.concat("</h2>");
  htmlText.concat("<br>");
  htmlText.concat("<h2>Number of steps taken=");
  htmlText.concat(String (counter));
  htmlText.concat("</h2>");




  //background-color: rgb(255,0,255);//background color
  //<svg height="210" width="500">
  //  <line x1="0" y1="0" x2="200" y2="200" style="stroke:rgb(255,0,0);stroke-width:2" />
  //</svg>
  //Begin line code
  //begin background color
  //htmlText.concat("div {background-size: 80px 60px;");
  //htmlText.concat("background-color: rgb(255,0,255); }");
  //end background color

  htmlText.concat("<table><tr> </tr> </table>");
  htmlText.concat("<svg height=");
  htmlText.concat('"');
  htmlText.concat("400");
  htmlText.concat("width=");
  htmlText.concat('"');
  htmlText.concat("800");
  htmlText.concat('"');
  htmlText.concat(">");


  htmlText.concat("<line x1=");
  htmlText.concat('"');
  htmlText.concat("0");
  htmlText.concat('"');

  htmlText.concat("y1=");
  htmlText.concat('"');
  htmlText.concat("200");
  htmlText.concat('"');


  htmlText.concat("x2=");
  htmlText.concat('"');
  htmlText.concat("800");
  htmlText.concat('"');

  htmlText.concat("y2=");
  htmlText.concat('"');
  htmlText.concat("200");
  htmlText.concat('"');

  htmlText.concat("style=");
  htmlText.concat('"');
  htmlText.concat("stroke:rgb(255,255,255);stroke-width:6");
  htmlText.concat('"');
  htmlText.concat("/>");
  //logo();
  htmlText.concat("</svg>");
  //end line code
  htmlText.concat("<br>");
  htmlText.concat("<br>");

  htmlText.concat("</body>");
  htmlText.concat("</html>");
  server.send(200, "text/html" , htmlText);



}

void setup() {

  delay(1000);
  Serial.begin(115200);
  pinMode(5, OUTPUT);
  pinMode(4, INPUT_PULLUP);//jumper for SSID
  pinMode(13, INPUT_PULLUP);//jumper for SSID
  pinMode(16, INPUT_PULLUP);//jumper for SSID
  pinMode(12, INPUT_PULLUP);// sets pin for tilt switch counter
  strcpy( ssid,  "Incredible Wearables");
  for (int checkPins = 0; checkPins < 5; checkPins++)
  {
    giveSSID();
  }

  //heartrate code
  Serial.println("Initializing...");



  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED


  //end heartrate code
  Serial.println();
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
  attachInterrupt (digitalPinToInterrupt (4), switchPressed, CHANGE);
}

void loop() {
  
  //heartrate code
  long irValue = particleSensor.getIR();
  temperature = particleSensor.readTemperature();
  temperatureF = particleSensor.readTemperatureF(); //Because I am a bad global citizen

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  if (irValue < 50000)
    Serial.print(" No finger?");

 
  server.handleClient();

  Serial.println(counter);
}
void giveSSID()
{

  delay(100);
  if (digitalRead(13) == LOW)
  {
    strcpy (ssid, "Incredible Wearables1");
  }
  delay(100);
  if (digitalRead(12) == LOW)//***change digitalRead(4) == LOW to digitalRead(12) == LOW
  {
    strcpy (ssid, "Incredible Wearables2");
  }
  delay(100);
  if (digitalRead(16) == LOW)
  {
    strcpy (ssid, "Incredible Wearables3");
  }

}


// Interrupt Service Routine (ISR)
void switchPressed ()
{
  if (digitalRead (4) == LOW)//**change digitalRead (12) == LOW to digitalRead (4) == LOW
  {
      state = 1;
      digitalWrite (5, HIGH);
    if (digitalRead (4) == LOW&&(millis()-last_interrupt>400))//***change digitalRead (12) == LOW&&(millis()-last_interrupt>400)
    {                                                          //** to digitalRead (4) == LOW&&(millis()-last_interrupt>400)
      counter++;
      state = 0;
      last_interrupt=millis();
    }
  }
  
  else
  {
    digitalWrite (5, LOW);
    state = 0;
  }
}  // end of switchPressed

