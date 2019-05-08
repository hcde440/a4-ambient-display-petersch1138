
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h";
#include <PubSubClient.h>

WiFiClient espClient;             // espClient object
PubSubClient mqtt(espClient);  // puubsubclient object
char mac[18]; //A MAC address is a 'truly' unique ID for each device, lets use that as our 'truly' unique user ID!!!
String url = "http://dataservice.accuweather.com/forecasts/v1/daily/1day/351409?apikey=" + weatherKey + "&language=en-us&details=true&metric=false";
//http://dataservice.accuweather.com/forecasts/v1/daily/1day/351409?apikey=TrmlypVFsbsvuGg5jkCQuQS9vR8VO85m&language=en-us&details=true&metric=false
float lux = 200.0;
float temp = 52.0; // initial values for testing
float chance = 25.0;
int BLUE = 13;
int GREEN = 14; // LED pins
int RED = 12;

// Peter Schultz May 7th 2019, HCDE 440, A4 mini project
// This script is receives data from two sensors, one for lux and one for temperature.
// It also gets data for precipitation chance from the Accuweather API. It turns on LEDs
// based on certain thresholds for each of the three data types. 

void setup() {
  // put your setup code here, to run once:
  pinMode(BLUE, OUTPUT);
  pinMode(GREEN, OUTPUT); // sets pin mode to output
  pinMode(RED, OUTPUT);
  digitalWrite(BLUE,HIGH);

  Serial.begin(115200);
  delay(100);
  Serial.print("This board is running: "); // the big 4
  Serial.println(F(__FILE__));
  Serial.print("Compiled: ");
  Serial.println(F(__DATE__ " " __TIME__));
  Serial.print("Connecting to "); Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA); // wifi mode
  WiFi.begin(WIFI_SSID, WIFI_PASS); // attempts to connect to wifi with given name and password

  while (WiFi.status() != WL_CONNECTED) { // prints dots until connected
    delay(500);
    Serial.print(".");
  }

  Serial.println(); Serial.println("WiFi connected"); Serial.println();
  mqtt.setServer(mqtt_server, 1883); // sets mqtt server
  mqtt.setCallback(callback); //register the callback function
  getWeather(); // connects to api and gets rain chance for the day

  reconnect(); // connects mqtt
}

void loop() {
  // put your main code here, to run repeatedly:
  
    if (!mqtt.connected()) {
      reconnect(); // run reconnect method if connection stops
    }
    mqtt.loop(); //this keeps the mqtt connection 'active'
  
  delay(100);
  check();
}

void getWeather() { // this method gets the wind speed in the given city from the weather API 
  HTTPClient theClient; // initializes browser
  theClient.begin(url); // client connects to given address, openweathermap.org
  int httpCode = theClient.GET(); // gets http response code
  if (httpCode > 0) { 

    if (httpCode == HTTP_CODE_OK) { // 200 means its working
      String payload = theClient.getString(); // gets the payload from the website (json format String)
      DynamicJsonBuffer jsonBuffer; // jsonbuffer will parse the payload
      JsonObject& root = jsonBuffer.parseObject(payload); // jsonObject contains the json data
      if (!root.success()) {
        Serial.println("parseObject() failed in getWeather().");
        return;
      }

      
      String dailyString = root["DailyForecasts"].as<String>(); // parse for daily forecast data
      // TRIM DAILY STRING OF BRACKETS
      dailyString.remove(0);
      dailyString.remove(dailyString.length() - 1);
      JsonObject& daily = jsonBuffer.parseObject(dailyString);
      String chanceString = daily["Day"]["PrecipitationProbability"].as<String>(); // parse json for precip probability
      chance = chanceString.toFloat();
      
      Serial.print("Chance of precipitation: ");
      Serial.println(chance);
      
    } else {
      Serial.print("HTTP Code: ");
      Serial.print(httpCode);
    }
  } else {
    Serial.printf("Something went wrong with connecting to the endpoint in getSpeed().");
  }
}

void check() { // this data checks every 100 ms if the LEDs should be turned on or off based on these cutoffs 
  if (lux > 1800.0) {
    digitalWrite(RED,HIGH);
  } else {
    digitalWrite(RED,LOW);
  }

  if (temp < 55.0) {
    digitalWrite(GREEN,HIGH);
  } else {
    digitalWrite(GREEN,LOW);
  }

  if(chance > 20.0) {
    digitalWrite(BLUE,LOW);
  } else {
    digitalWrite(BLUE,HIGH);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(mac, mqtt_user, mqtt_password)) { //<<---using MAC as client ID, always unique!!!
      Serial.println("connected");
      mqtt.subscribe("Peter/+"); //we are subscribing to 'fromPeter' and all subtopics below that topic
    } else {                        //please change 'fromPeter' to reflect your topic you are subscribing to
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void setup_wifi() { // this method connects to the wifi using the variables contained in config.h
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");  //get the unique MAC address to use as MQTT client ID, a 'truly' unique ID.
  WiFi.macAddress().toCharArray(mac, 18);
  Serial.println(mac);  //.macAddress returns a byte array 6 bytes representing the MAC address
}           

void callback(char* topic, byte* payload, unsigned int length) { // this method is called when an incoming message from a topic you are subscribed to is detected
  Serial.println();
  Serial.print("Message arrived [");
  Serial.print(topic); //'topic' refers to the incoming topic name, the 1st argument of the callback function
  Serial.println("] ");

  DynamicJsonBuffer  jsonBuffer; //creates json buffer
  JsonObject& root = jsonBuffer.parseObject(payload); //parse it!

  if (!root.success()) { 
    Serial.println("parseObject() failed, are you sure this message is JSON formatted.");
    return;
  }

  /////
  //We can use strcmp() -- string compare -- to check the incoming topic in case we need to do something
  //special based upon the incoming topic, like move a servo or turn on a light . . .
  //strcmp(firstString, secondString) == 0 <-- '0' means NO differences, they are ==
  /////

  if (strcmp(topic, "Peter/data") == 0) { // if its from Peter and the topic is data...
    Serial.println("A message from Peter . . .");
  }
  String luxS = root["lux"].as<String>(); // parse json data
  String tempS = root["temp"].as<String>();

  lux = luxS.toFloat(); // sets local variables
  temp = tempS.toFloat();
  root.printTo(Serial); //print out the parsed message
  Serial.println(); //give us some space on the serial monitor read out

}
