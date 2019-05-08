//Libraries to include
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_TSL2591.h"
#include <Adafruit_Si7021.h>
#include "config.h";

WiFiClient espClient;             // espClient object
PubSubClient mqtt(espClient);  // puubsubclient object
char mac[18]; //A MAC address is a 'truly' unique ID for each device, lets use that as our 'truly' unique user ID!!!
char message[201]; //201, as last character in the array is the NULL character, denoting the end of the array

float lux;
float temp; // local variables for light and temperature

//Some objects to instantiate
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591); // pass in a number for the sensor identifier (for your use later), lux sensor class
Adafruit_Si7021 sensor = Adafruit_Si7021(); // temp sensor class

//////////////////////////////////////SETUP
// Peter Schultz May 7th 2019, HCDE 440, A4 mini project
// This script is publishes data from two sensors, one for lux and one for temperature.
// The sensors are intended to be placed outdoors, and sends data via MQTT to a controller.
// The data is received by a controller board which turns on certain LEDs, one for each of
// the temp, light, and weather.
void setup()
{
  Serial.begin(115200);
  delay(100);
  Serial.print("This board is running: "); // the big 4
  Serial.println(F(__FILE__));
  Serial.print("Compiled: ");
  Serial.println(F(__DATE__ " " __TIME__));
  setup_wifi(); // sets up wifi
  mqtt.setServer(mqtt_server, 1883); // sets mqtt server

  reconnect(); // connects to mqtt
  
  /* Initialise the sensor */
  if (tsl.begin()) // initializes light sensor
  {
    Serial.println(F("Found a TSL2591 sensor"));
  } 
  else 
  {
    Serial.println(F("No TSL sensor found ... check your wiring?"));
    while (1);
  }

  if (!sensor.begin()) { // starts temp sensor
    Serial.println("Did not find Si7021 sensor!");
    while (true)
      ;
  }

}

////////////////////////////LOOP
void loop() {
  mqtt.loop(); // keeps mqtt connected

  if (!mqtt.connected()) {
    reconnect(); // called if mqtt is not connected
  }

  float temp = sensor.readTemperature() * 9 / 5 + 32; // converts temp data to Fahrenheit
  float light = tsl.getLuminosity(TSL2591_VISIBLE); // gets lux data

  Serial.print("temp: ");
  Serial.println(temp);
  Serial.print("lux: ");
  Serial.println(light);

  

  char str_temp[10];
  char str_light[10]; // arrays to hold the variables and message
  char message[50];

  dtostrf(temp, 4, 1, str_temp);
  dtostrf(light, 4, 0, str_light); // converts float to character array
  sprintf(message, "{\"temp\": \"%s\", \"lux\": \"%s\" }", str_temp, str_light); // puts char array into message
  Serial.println("publishing:");
  Serial.println(message);
  Serial.println("*************************************");  
  Serial.println("");
  mqtt.publish("Peter/data", message); // publishes message to the Peter/data topic
  delay(5000);
}

void reconnect() { // called to connect to mqtt
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(mac, mqtt_user, mqtt_password)) { //<<---using MAC as client ID, always unique!!!
      Serial.println("connected");
    } else {                        
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup_wifi() { // this method connects to the wifi
  delay(10);
  Serial.print("Connecting to "); Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA); // wifi mode
  WiFi.begin(WIFI_SSID, WIFI_PASS); // attempts to connect to wifi with given name and password

  while (WiFi.status() != WL_CONNECTED) { // prints dots until connected
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");  //get the unique MAC address to use as MQTT client ID, a 'truly' unique ID.
  WiFi.macAddress().toCharArray(mac, 18);
  Serial.println(mac); 
}
