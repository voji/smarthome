#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "SimpleTimer.h"

#include "auth.h"

//#define DEBUG
//#include "../common/debug.h"

#define RAIN_QUERYINTERVAL 10*1000  //ms
 
const String mqtt_mqttId = "wemos_"+mqtt_clientId;
const String mqtt_channelRelay = mqtt_mqttId+"_relay"; //in
const String mqtt_channelPing = mqtt_mqttId+"_ping"; //in
const String mqtt_channelOnline = mqtt_mqttId+"_online"; //out - wemos d1 goes online
const String mqtt_channelPong = mqtt_mqttId+"_pong"; //out
const String mqtt_channelRain = mqtt_mqttId+"_rain"; //out


//relay pins
const uint8_t relay_pins[] = {D3,D4,D6,D7,D8};
const uint8_t powerSupplyPin = D5;
const short relay_pins_size = sizeof(relay_pins) / sizeof(uint8_t);
uint8_t relay_pins_state[relay_pins_size+1];

//variables for analog smoothing
int inputPin = A0;
const int numReadings = 10;
int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int lastAverage = 0;

WiFiClient wifiClient;

SimpleTimer timer;

void mqttCallback(char* topic, byte* payload, unsigned int msglength) {  
  String topicStr = String(topic);
  boolean b = LOW;
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
        cdigitalWrite(relay,pinVMode); 
      }
  } else if (mqtt_channelPing.equals(topicStr)) {
    //DPN("Scheduling pong response");
    timer.setTimeout(3000, callbackPong);
  } else {
    //DPN("Unknown topic: "+ topicStr);
  }
}

PubSubClient mqttClient(mqtt_ip, mqtt_port, mqttCallback,wifiClient);


void cdigitalWrite(int pin, uint8_t pmode) {
  if (relay_pins_state[pin] != pmode) {
    int relayGpioNr = relay_pins[pin];
    //DPN("Changing pin state. Id: " + String(pin) + " GPIO: " + String(relayGpioNr) + " Mode: " + String(pmode));
    digitalWrite(relayGpioNr,pmode); 
    relay_pins_state[pin] = pmode;

    if (pmode == LOW) {
      if (relay_pins_state[relay_pins_size] != LOW) {
        digitalWrite(powerSupplyPin,LOW); 
        relay_pins_state[relay_pins_size] = LOW;
        //DPN("Relay turned on, turning on PwrSupply");        
      }
    } else {
      //DPN("Relay turned off, scheduling PwrSupply state check");
      timer.setTimeout(10000, callbackPowerDown);
    }
  } else {
    //DPN("Pin change request ignored (no change). Id: " + String(pin) + " Mode: " + String(pmode));
  }
}


void connectMqtt() {
  boolean ledState = true;  
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
    WiFi.begin(ssid, password);
    WiFi.config(wifi_ip, IPAddress(192,168,1,254), IPAddress(255,255,255,0), IPAddress(8,8,8,8),IPAddress(8,8,4,4));
    //wait 5 sec
    delay(5000);
  }
  //DPN("Wifi connected");
}


void resetRelays() {
    //DPN("Resetting relay pins...");
    for (int i=0; i<relay_pins_size; i++) {    
      //DPN("Initializing pin: " + String(i));
      cdigitalWrite(i,HIGH);
  }
}

void setup() {
#ifdef DEBUG
  Serial.begin(115200);  
#endif
  //DPN("Start wemos board setup...");
  //DPN("Initializing Relay pins... ItemNr: "+ String());
  for (int i=0; i<relay_pins_size; i++) {    
    //DPN("Initializing pin: " + String(relay_pins[i]));
    pinMode(relay_pins[i], OUTPUT);
    relay_pins_state[i] = HIGH;
  }
  pinMode(powerSupplyPin, OUTPUT);
  relay_pins_state[relay_pins_size] = HIGH;
  
  timer.setInterval(RAIN_QUERYINTERVAL, callbackRainDetect);
}

bool checkTolerance(float cVal, float oVal, float tol) {
  float diff = cVal - oVal;
  return abs(diff) > tol;
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

void callbackRainDetect() {
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
      //DPN("Current rain sensor temp voltage: "+ (String(average)));
      if (checkTolerance(average, lastAverage, 12)) {
        lastAverage = average;
        mqttClient.publish(mqtt_channelRain.c_str(), (String(average)).c_str());
        //DPN("Rain sensor value changed and published");
      } else {
        //DPN("Rain sensor value not changed");
      }
    }
}

void callbackPowerDown() {
    //DPN("Processing PwrSupply state check");
    for (int i = 0; i<relay_pins_size; i++) {
      if (relay_pins_state[i] == LOW) {
       return;
      }
    }
    if (relay_pins_state[relay_pins_size] != HIGH) {
      //DPN("PwrSupply state changed to mode: " + String(tpwrstate));
      digitalWrite(powerSupplyPin,HIGH);
      relay_pins_state[relay_pins_size] = HIGH;
    } else {
      //DPN("PwrSupply state not changed");
    }
}

void callbackPong() {
   ////DPN("Sending pong response");
   mqttClient.publish(mqtt_channelPong.c_str(), (String("pong")).c_str());
}

