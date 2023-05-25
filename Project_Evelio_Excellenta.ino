//Evelio Excellenta
//NPM : 2006579705

#include <WiFiClientSecure.h>
#include "TRIGGER_GOOGLESHEETS_MODIF.h"
#include <PubSubClient.h>
#include <DHT.h>
#include <MQ2.h>


//WIFI
WiFiClient espClient;
const char* ssid = "EM-10";
const char* password = "C0nn3ctwifi";
const char* mqtt_server = "broker.mqtt-dashboard.com";
PubSubClient client(espClient);
     

//MQTT =======================================
const char* MQTTRelay01 = "1-EM10C4/12/Relay";   //lampu otomatis
const char* MQTTRelay02 = "2-EM10C4/12/Relay";   //dispenser 
const char* MQTTDHTTemp = "EM10C4/12/DHT/Temp"; 
const char* MQTTDHTHum = "EM10C4/12/DHT/Hum";
const char* MQTTPIR01 = "1-EM10C4/12/PIR";       
const char* MQTTLPG01 = "1-EM10C4/12/LPG";
const char* MQTTCO01 = "1-EM10C4/12/CO";
const char* MQTTSmoke01 = "1-EM10C4/12/Smoke";

// NodeMCU PIN
#define pinRelay01 D0
#define pinRelay02 D1
#define DHTPIN D4
#define pinPIR01 D5
#define pinMQ201 A0

//Initial Status Relays
bool R01on = LOW;
bool R02on = LOW;

int val = 0;

int i=0; //variabel menghitung jumlah loop

//MQ2 =======
float lpg, co, smoke;
MQ2 mq2(pinMQ201);

//PIR ====
float statusPIR;

//DHT ========================================
       
#define DHTTYPE DHT22     
float h_temp,t_temp;
float hh,tt;
DHT dht(DHTPIN, DHTTYPE);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50) //Menyimpan string 
char msg[MSG_BUFFER_SIZE];

/**********Google Sheets Definations***********/
char column_name_in_sheets[ ][20] = {"value1","value2","value3"};                        
String Sheets_GAS_ID = "AKfycbz8jX-TeY-NBsjLbrOmCDAeYSL4Vbdn9EzIhAGJNHYmQLPMXexr";                                   
int No_of_Parameters = 3;                                                               
/*********************************************/

void sendSensor(float t, float h){
  int i = 0;
    
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.println (t_temp);
  if (t_temp != t){
    t_temp = t;
    client.publish(MQTTDHTTemp, String(t).c_str(),true);
  }
  
  Serial.println (h_temp);
  if (h_temp != h){
    h_temp = h;
    client.publish(MQTTDHTHum, String(h).c_str(),true);
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) { 
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (topic[0] == MQTTRelay01[0]){
     if ((char)payload[0] == '1') {
      digitalWrite (pinRelay01,R01on);
     } else {
      digitalWrite (pinRelay01,!R01on);
     }
  }
  else if (topic[0] == MQTTRelay02[0]) {
    if ((char)payload[0] == '1') {
      digitalWrite (pinRelay02,R02on);
     } else {
      digitalWrite (pinRelay02,!R02on);
     }
  } 
}

void reconnect() {
  while (!client.connected()) {
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      client.subscribe(MQTTRelay01);
      client.subscribe(MQTTRelay02);
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Google_Sheets_Init(column_name_in_sheets, Sheets_GAS_ID, No_of_Parameters );               

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  pinMode (pinRelay01,OUTPUT);
  pinMode (pinRelay02,OUTPUT);
  pinMode (pinPIR01, INPUT);
  
  digitalWrite (pinRelay01,!R01on);
  digitalWrite (pinRelay02,!R02on);  

  mq2.begin();
  dht.begin();

}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }

  unsigned long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now;

    if (!client.connected()) {
      reconnect();
    }
    client.loop();

    //DHT
    hh = dht.readHumidity();
    tt = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit

    if ((h_temp != hh) || (t_temp != tt)){
      sendSensor(tt,hh);
    }

    //GAS
    lpg = mq2.readLPG();
    co = mq2.readCO();
    smoke = mq2.readSmoke();

    Serial.print("LPG: ");
    Serial.println(lpg);

    snprintf (msg, MSG_BUFFER_SIZE, "%2d", lpg);    
    client.publish(MQTTLPG01, String(lpg).c_str(),true);
 
    Serial.print("CO: ");
    Serial.println(co);
    snprintf (msg, MSG_BUFFER_SIZE, "%2d", co);   
    client.publish(MQTTCO01, String(co).c_str(),true);

    Serial.print("Smoke: ");
    Serial.println(smoke);
    snprintf (msg, MSG_BUFFER_SIZE, "%2d", smoke);  
    client.publish(MQTTSmoke01, String(smoke).c_str(),true);

  
  //PIR
    val = digitalRead(pinPIR01);
    Serial.print ("PIR : ");
    Serial.println (val);
    if(val == HIGH){    
      delay(100);
        Serial.println("Motion Detected!");  
        statusPIR=1;       
        client.publish(MQTTPIR01,"1"); 
        client.publish(MQTTRelay01,"1");   
        
    } else {
      delay(100);
        Serial.println("Motion stopped!");
        statusPIR=0;
        client.publish(MQTTPIR01,"0"); 
        client.publish(MQTTRelay01, "0");
      
    }
    
  }
  
  i++;
  Serial.println(i);
  if(i>=10){
    Data_to_Sheets(No_of_Parameters,  tt,  lpg, statusPIR);  
    i=0;
  }
  

}
