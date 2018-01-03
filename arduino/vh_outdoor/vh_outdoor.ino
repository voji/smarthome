#include <ESP8266WiFi.h>
#include <Wire.h>
#include <SHT21.h>
#include <PubSubClient.h>
#include "SimpleTimer.h"

#include "auth.h"

//#define DEBUG
//#include "debug.h"

#define SHT_QUERYINTERVAL 60*1000  //ms
#define SENSOR_QUERYINTERVAL 500  //ms
const int sensorPin = D8;


const String mqtt_clientId = "outdoor";
const String mqtt_mqttId = "wemos_"+mqtt_clientId;
const String mqtt_channelRelay = mqtt_mqttId+"_relay"; //in
const String mqtt_channelPing = mqtt_mqttId+"_ping"; //in
const String mqtt_channelResetTC = mqtt_mqttId+"_resettc"; //in - reset temperature cache
const String mqtt_channelOnline = mqtt_mqttId+"_online"; //out - wemos d1 goes online
const String mqtt_channelPong = mqtt_mqttId+"_pong"; //out
const String mqtt_channelTemp = mqtt_mqttId+"_temp"; //out
const String mqtt_channelHum = mqtt_mqttId+"_hum";  //out
const String mqtt_channelSensor = mqtt_mqttId+"_movementsensor";  //out

boolean lastSensorState = false;
float temp = 0.0;
float hum = 0.0;
int invalidTemp = 0;

WiFiClient wifiClient;
SHT21 SHT21;
SimpleTimer timer;

void mqttCallback(char* topic, byte* payload, unsigned int msglength) {  
  String topicStr = String(topic);
  //DPN("Message arrived [ " + topicStr + "] ");
  if (mqtt_channelPing.equals(topicStr)) {
    //DPN("Scheduling pong response");
    timer.setTimeout(3000, callbackPong);
  } else if (mqtt_channelResetTC.equals(topicStr)) {
    //DPN("Cleaning cached temperature values");
    temp = 0.0;
    hum = 0.0;
  } else {
    //DPN("Unknown topic: "+ topicStr);
  }
}

PubSubClient mqttClient(mqtt_ip, mqtt_port, mqttCallback,wifiClient);


void connectMqtt() {
  while (!mqttClient.connected()) {
    //DPN("Attempting MQTT connection...");
    if (mqttClient.connect(mqtt_mqttId.c_str(), mqtt_user, mqtt_password)) {
      //DPN("MQTT connected");
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
    //DPN("Trying to connect SSID: " + String(wifi_ssid));
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_password);
    WiFi.config(wifi_ip, IPAddress(192,168,1,254), IPAddress(255,255,255,0), IPAddress(8,8,8,8),IPAddress(8,8,4,4));
    delay(5000);
  }
  //DPN("Wifi connected");
}

bool checkTolerance(float cVal, float oVal, float tol) {
  float diff = cVal - oVal;
  return abs(diff) > tol;
}


void setup() {
#ifdef DEBUG
  Serial.begin(115200);  
#endif
  //these delays required, otherwise wifi hangs on connect 
  delay(1000);
  //DPN("Start wemos board setup...");
  //DPN("Pinmode init...");
  pinMode(sensorPin, INPUT);
  delay(500);
  //DPN("SHT init...");
  SHT21.begin();
  delay(500);
  //DPN("Timer setup...");
  timer.setInterval(SHT_QUERYINTERVAL, callbackReadSHT);
  timer.setInterval(SENSOR_QUERYINTERVAL, callbackReadSensor);
}

void loop() {
   if (WiFi.status() != WL_CONNECTED) {
    connectWifi();
    return;
   }
   
   if (!mqttClient.connected()) {
    connectMqtt();
    return;
   }
   
   mqttClient.loop();

   timer.run();   
}

void callbackReadSHT() {
    //DPN("Reading DHT values");
    float t, h; 
    t = SHT21.getTemperature();
    h = SHT21.getHumidity();
    
    //DPN("Current temperature: "+ (String(t, 1)));
    //DPN("Current humidity: "+ (String(h, 1)));
          
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
}

void callbackReadSensor() {
  delay(100);
  int sensorresult = digitalRead(sensorPin);
  if (sensorresult == LOW && lastSensorState) {
    mqttClient.publish(mqtt_channelSensor.c_str(), String("OFF").c_str());
    lastSensorState = false;
  } else if (sensorresult == HIGH && !lastSensorState) {
    mqttClient.publish(mqtt_channelSensor.c_str(), String("ON").c_str());
    lastSensorState = true;
  }
}

void callbackPong() {
   //DPN("Sending pong response");
   mqttClient.publish(mqtt_channelPong.c_str(), (String("pong")).c_str());
}


