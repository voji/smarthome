#include <Esp.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "PietteTech_DHT.h" 
#include "SimpleTimer.h"

#include "auth.h"

//#define DEBUG
//#include "debug.h"

//DHT config
#define DHTTYPE  AM2302           // Sensor type DHT11/21/22/AM2301/AM2302
#define DHTPIN   D3              // Digital pin for communications

#define QUERYINTERVAL 10*1000  //ms


//mqtt
const String mqtt_clientId = "dungeon";
const String mqtt_mqttId = "wemos_"+mqtt_clientId;
const String mqtt_channelRelay = mqtt_mqttId+"_relay"; //in
const String mqtt_channelPing = mqtt_mqttId+"_ping"; //in
const String mqtt_channelResetTC = mqtt_mqttId+"_resettc"; //in - reset temperature cache
const String mqtt_channelOnline = mqtt_mqttId+"_online"; //out - wemos d1 goes online
const String mqtt_channelPong = mqtt_mqttId+"_pong"; //out
const String mqtt_channelTemp = mqtt_mqttId+"_temp"; //out
const String mqtt_channelHum = mqtt_mqttId+"_hum";  //out
const String mqtt_channelBoiler = mqtt_mqttId+"_boiler";  //out


//relay pins
const int relay_pins[] = {D4,D5,D6,D7};
const int relay_pins_size = sizeof(relay_pins) / sizeof(int);

float temp = 0.0;
float hum = 0.0;
int invalidTemp = 0;

//variables for analog smoothing
int inputPin = A0;
const int numReadings = 3;
int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int lastAverage = 0;
int temperatureCheck = 0;


WiFiClient wifiClient;

void dht_wrapper();

SimpleTimer timer;

PietteTech_DHT DHT(DHTPIN, DHTTYPE, dht_wrapper);
 
void dht_wrapper() {
  DHT.isrCallback();
} 


PubSubClient mqttClient(mqtt_ip, mqtt_port, mqttCallback,wifiClient);

void connectMqtt() {
  while (!mqttClient.connected()) {
    //DPN("Attempting MQTT connection...");
    if (mqttClient.connect(mqtt_mqttId.c_str(), mqtt_user, mqtt_password)) {
      //DPN("MQTT connected");
      for (int i=0;i<relay_pins_size; i++) {
        String relayTopic = mqtt_channelRelay + i;
        //DPN("Subscribing: " + relayTopic);        
        mqttClient.subscribe(relayTopic.c_str());
      }
      //DPN("Subscribing: " + mqtt_channelPing);
      mqttClient.subscribe(mqtt_channelPing.c_str());
      mqttClient.subscribe(mqtt_channelResetTC.c_str());
      mqttClient.publish(mqtt_channelOnline.c_str(), String("ON").c_str());
    } else {
      //DPN("failed, rc=" + String(mqttClient.state()) + " try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void connectWifi() { 
  while (WiFi.status() != WL_CONNECTED) {
    //DPN("Trying to connect SSID: " + String(ssid));
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_password);
    WiFi.config(wifi_ip, IPAddress(192,168,1,254), IPAddress(255,255,255,0), IPAddress(8,8,8,8),IPAddress(8,8,4,4));
    //wait 15 sec
    delay(5000);
  }
  //DPN("Wifi connected");
}

bool checkTolerance(float cVal, float oVal, float tol) {
  float diff = cVal - oVal;
  return abs(diff) > tol;
}

void resetRelays() {
    //DPN("Resetting relay pins...");
    for (int i=0; i<relay_pins_size; i++) {    
      //DPN("Turning off pin: " + String(relay_pins[i]));
      digitalWrite(relay_pins[i],HIGH); 
  }
}


void setup() {
#ifdef DEBUG
  Serial.begin(115200);  
#endif
  //DPN("Start wemos board setup...");
  //DPN("Initializing analog smoothing array...");
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
  //DPN("Initializing Relay pins...");
  for (int i=0; i<relay_pins_size; i++) {    
    //DPN("Initializing pin: " + String(relay_pins[i]));
    pinMode(relay_pins[i], OUTPUT);    
  }

  timer.setInterval(QUERYINTERVAL, callbackSensorRead);
}


void loop() {
   if (WiFi.status() != WL_CONNECTED) {
    resetRelays();
    connectWifi();
   }
   
   if (!mqttClient.connected()) {
    resetRelays();
    connectMqtt();
   }
   
   mqttClient.loop();
   timer.run();
}

void mqttCallback(char* topic, byte* payload, unsigned int msglength) {  
  String topicStr = String(topic);
  //DPN("Message arrived [ " + topicStr + "] ");
  if (topicStr.startsWith(mqtt_channelRelay)) {
      char relayIdChar=topic[mqtt_channelRelay.length()];
      //DPN("RelayIdChar: " + String(relayIdChar));      
      int relay = relayIdChar-'0';
      if (relay<0 || relay>relay_pins_size) {
        //DPN("Invalid relay id: " + String(relay));
      } else {
        int pinVMode = HIGH;
        if (msglength==2) {
          //DPN("Switch mode chnged to ON");
          pinVMode = LOW;
        }
        //DPN("Switching relay. GPIO: " + String(relay_pins[relay]) + " pinVmode: " + String(pinVMode));
        digitalWrite(relay_pins[relay],pinVMode);
      }
  } else if (mqtt_channelPing.equals(topicStr)) {
    //DPN("Scheduling pong response");
    timer.setTimeout(3000, callbackPong);
  } else if (mqtt_channelResetTC.equals(topicStr)) {
    //DPN("Cleaning cached temperature values");
    temp = 0.0;
    hum = 0.0;
    lastAverage = 0;
  } else {
    //DPN("Unknown topic: "+ topicStr);
  }
}

void callbackSensorRead() {
  if (temperatureCheck<6) {
    callbackReadBoiler();
  } else {
    callbackReadDHT();
    temperatureCheck = 0;
  }
  temperatureCheck++;
}

void callbackReadDHT() {
    //DPN("Reading DHT values");
    int acquireresult = -1;
    acquireresult = DHT.acquireAndWait(1000);
    float t, h, d; 
    if ( acquireresult == 0 ) {
      t = DHT.getCelsius();
      h = DHT.getHumidity();
      //d = DHT.getDewPoint();
      
      //DPN("Current temperature: "+ (String(t, 1)));
      //DPN("Current humidity: "+ (String(h, 1)));

      if (t < 0) {
      //DPN("Current temperature is invalid: "+ (String(t, 1)));
        invalidTemp++;
        if (invalidTemp > 5) {
          //DPN("Too many invalid temperature readed, restartig...");
          invalidTemp = 0;
          ESP.reset();
        }
      } else {
        invalidTemp = 0;
      }
      
      if (checkTolerance(t, temp, 0.5)) {
        temp = t;
        mqttClient.publish(mqtt_channelTemp.c_str(), (String(t, 1)).c_str());
        //DPN("Temperature changed, and published");
      } else {
        //DPN("Temperature not changed");
      }
  
      if (checkTolerance(h, hum, 0.5)) {
        hum = h;
        mqttClient.publish(mqtt_channelHum.c_str(), (String(h, 1)).c_str());
        //DPN("Humidity changed, and published");
      } else {
        //DPN("Humidity not changed");
      }
    } else {
      //DPN("Unable to read temperature sensor. Result: "+ String(acquireresult));
    }
}

void callbackReadBoiler() {
  //DPN("Reading boiler temp");  
  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = analogRead(inputPin);
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;  

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    //DPN("Processing readed analog values: "+ (String(readIndex)));
    // ...wrap around to the beginning:
    readIndex = 0;
    int average = total / numReadings;
    //DPN("Current boiler temp voltage: "+ (String(average)));
    if (checkTolerance(average, lastAverage, 2)) {
      lastAverage = average;
      float voltage = ((float)average/1024.f)*3300.f;      
      float cel =  (voltage-500.f) / 10.f;
      mqttClient.publish(mqtt_channelBoiler.c_str(), (String(cel, 1)).c_str());
      //DPN("Boiler temp changed and published");
    } else {
      //DPN("Boiler temp not changed");
    }
  }
}

void callbackPong() {
   //DPN("Sending pong response");
   mqttClient.publish(mqtt_channelPong.c_str(), (String("pong")).c_str());    
}

