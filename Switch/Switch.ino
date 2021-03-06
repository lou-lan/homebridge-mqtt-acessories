// Includes //
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include "Adafruit_MCP23017.h"
#define DHTTYPE DHT11
#define DHTPIN  16

const char* ssid = "HomeBridge202";
const char* password = "HomeBridgePassPhrase";
uint chipId_int;
uint16_t i;
char serviceType[128] = "Switch";
int numSwitches = 1; //1-4
bool contactSensorPresent = false;
bool tempHumiSensorPresent = false;
char chipId[128];
char chipIdA[128];
char chipIdB[128];
char chipIdC[128];
char chipIdD[128];
char chipIdE[128];
char chipIdF[128];
char chipIdG[128];
char pubTopic[128];
char pubMessage[256];

const char* mqttpass = "";
const char* intopic = "homebridge/from/set";
const char* outtopic = "homebridge/to/set";
const char* gettopic = "homebridge/from/get";
const char* addtopic = "homebridge/to/add";
const char* removetopic = "homebridge/to/remove";
const char* mainttopic = "homebridge/from/connected";
const char* maintmessage = "";
const char* accessoryName;
const char* accessoryCharacteristic;
const char* accessoryValue;
const char* accessoryValueString;
bool currentSwitchAState;
bool currentSwitchBState;
bool currentSwitchCState;
bool currentSwitchDState;
bool currentContactSensorState;
bool oldSwitchAState;
bool oldSwitchBState;
bool oldSwitchCState;
bool oldSwitchDState;
int oldContactSensorState;
bool digitalSwitchAState;
bool digitalSwitchBState;
bool digitalSwitchCState;
bool digitalSwitchDState;
bool switchState;
int contactSensorState;
bool switchAState;
bool switchBState;
bool switchCState;
bool switchDState;

String chipId_stringA;
String chipId_stringB;
String chipId_stringC;
String chipId_stringD;
String chipId_stringE;
String chipId_stringF;
String chipId_stringG;

unsigned long lastDebounceATime = 0;
unsigned long lastDebounceBTime = 0;
unsigned long lastDebounceCTime = 0;
unsigned long lastDebounceDTime = 0;
unsigned long debounceDelay = 60;

Adafruit_MCP23017 mcp;

IPAddress server(10, 1, 0, 1);
WiFiClient wclient;
PubSubClient client(wclient, server);
DHT dht(DHTPIN, DHTTYPE);

float celsiusTemp, farenheitTemp, humidityTemp;
unsigned long previousMillis = 0;
unsigned long previousTempMillis = 0;

const long interval = 2000;
unsigned long lasttemphumipublish = 0;
const long temphumipublishtime = 15000;

void setup() {
    Serial.begin(74880);
    String chipId_string;
    chipId_string = serviceType+String(ESP.getChipId());
    chipId_string.toCharArray(chipId,128);
    if(numSwitches == 1 || numSwitches == 2 || numSwitches == 3 || numSwitches == 4){
    chipId_stringA = "SwitchA"+String(ESP.getChipId());
    chipId_stringA.toCharArray(chipIdA,128);
    #define ledAPin 14
    #define switchAPin 13
    #define relayAPin 12
    pinMode(ledAPin, OUTPUT);
    pinMode(switchAPin, INPUT);
    pinMode(relayAPin, OUTPUT);
    digitalWrite(ledAPin, LOW);
    digitalWrite(relayAPin, LOW);
    digitalSwitchAState = false;
    switchAState = false;
    oldSwitchAState = false;
    Serial.println("Switch 'A' Ready");
    if(numSwitches == 2 || numSwitches == 3 || numSwitches == 4){
    chipId_stringB = "SwitchB"+String(ESP.getChipId());
    chipId_stringB.toCharArray(chipIdB,128);
    mcp.begin();
    #define ledBPin 1
    #define switchBPin 5
    mcp.pinMode(ledBPin, OUTPUT);
    mcp.pinMode(switchBPin, INPUT);
    mcp.digitalWrite(ledBPin, LOW);
    digitalSwitchBState = false;
    switchBState = false;
    oldSwitchBState = false;
    if(numSwitches == 3 || numSwitches == 4){
    chipId_stringC = "SwitchC"+String(ESP.getChipId());
    chipId_stringC.toCharArray(chipIdC,128);
    #define ledCPin 2
    #define switchCPin 6
    mcp.pinMode(ledCPin, OUTPUT);
    mcp.pinMode(switchCPin, INPUT);
    mcp.digitalWrite(ledCPin, LOW);
    digitalSwitchCState = false;
    switchCState = false;
    oldSwitchCState = false;
    if(numSwitches == 4){
    chipId_stringD = "SwitchD"+String(ESP.getChipId());
    chipId_stringD.toCharArray(chipIdD,128);
    #define ledDPin 3
    #define switchDPin 7
    mcp.pinMode(ledDPin, OUTPUT);
    mcp.pinMode(switchDPin, INPUT);
    mcp.digitalWrite(ledDPin, LOW);
    digitalSwitchDState = false;
    switchDState = false;
    oldSwitchDState = false;
    }
    }
    }
    }
    if(contactSensorPresent == true){
      chipId_stringE = "ContactSensor"+String(ESP.getChipId());
      chipId_stringE.toCharArray(chipIdE,128);
      #define contactPin 5
      pinMode(contactPin, INPUT);
      oldContactSensorState = digitalRead(contactPin);
      Serial.println("Contact True");
    }
    if(tempHumiSensorPresent == true){
      chipId_stringF = "TemperatureSensor"+String(ESP.getChipId());
      chipId_stringF.toCharArray(chipIdF,128);
      chipId_stringG = "HumiditySensor"+String(ESP.getChipId());
      chipId_stringG.toCharArray(chipIdG,128);
      dht.begin();
      Serial.println("DHT True");
    }
    wifi_conn();
    ArduinoOTA.setPort(8266);
    ArduinoOTA.setHostname(chipId);
    // ArduinoOTA.setPassword((const char *)"123");
    ArduinoOTA.onStart([]() {
    Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
    ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
}

void wifi_conn(){
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.println();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop(){
    ArduinoOTA.handle();
    if(numSwitches == 1 || numSwitches == 2 || numSwitches == 3 || numSwitches == 4){
    currentSwitchAState = digitalRead(switchAPin);
    if(currentSwitchAState != oldSwitchAState){
        lastDebounceATime = millis();
      }
    if ((millis() - lastDebounceATime) > debounceDelay) {
        if (currentSwitchAState != switchAState) {
          switchAState = currentSwitchAState;
          if (switchAState == true) {
          digitalSwitchAState = !digitalSwitchAState;
          StaticJsonBuffer<200> switchABuffer;
          JsonObject& switchAJson = switchABuffer.createObject();
          switchAJson["name"] = chipIdA;
          switchAJson["characteristic"] = "On";
          switchAJson["value"] = digitalSwitchAState;
          String switchAString;
          switchAJson.printTo(switchAString);
          client.publish(outtopic,switchAString);
          }
        }
      }
      oldSwitchAState = currentSwitchAState;
      if(digitalSwitchAState == true){
        digitalWrite(ledAPin, HIGH);
        digitalWrite(relayAPin, HIGH);
      }else{
        digitalWrite(ledAPin, LOW);
        digitalWrite(relayAPin, LOW);
      }
      if(numSwitches == 2 || numSwitches == 3 || numSwitches == 4){
        currentSwitchBState = mcp.digitalRead(switchBPin);
        if(currentSwitchBState != oldSwitchBState){
          lastDebounceBTime = millis();
          }
          if ((millis() - lastDebounceBTime) > debounceDelay) {
            if (currentSwitchBState != switchBState) {
              switchBState = currentSwitchBState;
              if (switchBState == true) {
                digitalSwitchBState = !digitalSwitchBState;
                StaticJsonBuffer<200> switchBBuffer;
                JsonObject& switchBJson = switchBBuffer.createObject();
                switchBJson["name"] = chipIdB;
                switchBJson["characteristic"] = "On";
                switchBJson["value"] = digitalSwitchBState;
                String switchBString;
                switchBJson.printTo(switchBString);
                client.publish(outtopic,switchBString);
              }
            }
          }
          oldSwitchBState = currentSwitchBState;
          if(digitalSwitchBState == true){
            mcp.digitalWrite(ledBPin, HIGH);
            }
            else{
              mcp.digitalWrite(ledBPin, LOW);
              }
              if(numSwitches == 3 || numSwitches == 4){
                currentSwitchCState = mcp.digitalRead(switchCPin);
                if(currentSwitchCState != oldSwitchCState){
                  lastDebounceCTime = millis();
                  }
                  if ((millis() - lastDebounceCTime) > debounceDelay) {
        if (currentSwitchCState != switchCState) {
          switchCState = currentSwitchCState;
          if (switchCState == true) {
          digitalSwitchCState = !digitalSwitchCState;
          StaticJsonBuffer<200> switchCBuffer;
          JsonObject& switchCJson = switchCBuffer.createObject();
          switchCJson["name"] = chipIdC;
          switchCJson["characteristic"] = "On";
          switchCJson["value"] = digitalSwitchCState;
          String switchCString;
          switchCJson.printTo(switchCString);
          client.publish(outtopic,switchCString);
          }
        }
      }
      oldSwitchCState = currentSwitchCState;
      if(digitalSwitchCState == true){
        mcp.digitalWrite(ledCPin, HIGH);
      }else{
        mcp.digitalWrite(ledCPin, LOW);
      }
      if(numSwitches == 4){
        currentSwitchDState = mcp.digitalRead(switchDPin);
      if(currentSwitchDState != oldSwitchDState){
        lastDebounceDTime = millis();
      }
      if ((millis() - lastDebounceDTime) > debounceDelay) {
        if (currentSwitchDState != switchDState) {
          switchDState = currentSwitchDState;
          if (switchCState == true) {
          digitalSwitchDState = !digitalSwitchDState;
          StaticJsonBuffer<200> switchDBuffer;
          JsonObject& switchDJson = switchDBuffer.createObject();
          switchDJson["name"] = chipIdC;
          switchDJson["characteristic"] = "On";
          switchDJson["value"] = digitalSwitchDState;
          String switchDString;
          switchDJson.printTo(switchDString);
          client.publish(outtopic,switchDString);
          }
        }
      }
      oldSwitchDState = currentSwitchDState;
      if(digitalSwitchDState == true){
        mcp.digitalWrite(ledDPin, HIGH);
      }else{
        mcp.digitalWrite(ledDPin, LOW);
      }
      }
                  }
                  }
                  }
    if(contactSensorPresent == true){
      currentContactSensorState = digitalRead(contactPin);
          if(currentContactSensorState == true){
          contactSensorState = 1;
          }
          else if(currentContactSensorState == false){
          contactSensorState = 0;
          }
          if(currentContactSensorState != oldContactSensorState){
        StaticJsonBuffer<200> contactBuffer;
        JsonObject& contactJson = contactBuffer.createObject();
        contactJson["name"] = chipIdE;
        contactJson["characteristic"] = "ContactSensorState";
        contactJson["value"] = contactSensorState;
        String contactString;
        contactJson.printTo(contactString);
        client.publish(outtopic,contactString);
        oldContactSensorState = currentContactSensorState;
    }
    }
    if(tempHumiSensorPresent == true){
      unsigned long currentTempMillis = millis();
      gettemperature();
      if(currentTempMillis - previousTempMillis >= temphumipublishtime){
        previousTempMillis = currentTempMillis;
        publishtemperature();
      }
    }
    if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      if (client.connect(MQTT::Connect(chipId)
       .set_auth(chipId, mqttpass))) {
  client.set_callback(callback);
  client.subscribe(intopic);
  client.subscribe(gettopic);
  client.subscribe(mainttopic);
  addAccessory();
      } else {
        delay(5000);
      }
    }

    if (client.connected())
      client.loop();
  } else {
    wifi_conn();
  }
}

void addAccessory(){
  if(numSwitches == 1 || numSwitches == 2 || numSwitches == 3 || numSwitches == 4){
    StaticJsonBuffer<200> jsonAddSwitchABuffer;
    JsonObject& addSwitchAJson = jsonAddSwitchABuffer.createObject();
    addSwitchAJson["name"] = chipIdA;
    addSwitchAJson["service"] = "Switch";
    String addSwitchAString;
    addSwitchAJson.printTo(addSwitchAString);
    client.publish(addtopic,addSwitchAString);
    if(numSwitches == 2 || numSwitches == 3 || numSwitches == 4){
    StaticJsonBuffer<200> jsonAddSwitchBBuffer;
    JsonObject& addSwitchBJson = jsonAddSwitchBBuffer.createObject();
    addSwitchBJson["name"] = chipIdB;
    addSwitchBJson["service"] = "Switch";
    String addSwitchBString;
    addSwitchBJson.printTo(addSwitchBString);
    client.publish(addtopic,addSwitchBString);
    if(numSwitches == 3 || numSwitches == 4){
    StaticJsonBuffer<200> jsonAddSwitchCBuffer;
    JsonObject& addSwitchCJson = jsonAddSwitchCBuffer.createObject();
    addSwitchCJson["name"] = chipIdC;
    addSwitchCJson["service"] = "Switch";
    String addSwitchCString;
    addSwitchCJson.printTo(addSwitchCString);
    client.publish(addtopic,addSwitchCString);
    if(numSwitches == 4){
    StaticJsonBuffer<200> jsonAddSwitchDBuffer;
    JsonObject& addSwitchDJson = jsonAddSwitchDBuffer.createObject();
    addSwitchDJson["name"] = chipIdD;
    addSwitchDJson["service"] = "Switch";
    String addSwitchDString;
    addSwitchDJson.printTo(addSwitchDString);
    client.publish(addtopic,addSwitchDString);
  }
    }
    }
  }
  if(contactSensorPresent == true){
    StaticJsonBuffer<200> jsonAddContactSensorBuffer;
    JsonObject& addContactSensorJson = jsonAddContactSensorBuffer.createObject();
    addContactSensorJson["name"] = chipIdE;
    addContactSensorJson["service"] = "ContactSensor";
    String addContactSensorString;
    addContactSensorJson.printTo(addContactSensorString);
    client.publish(addtopic,addContactSensorString);
  }
  if(tempHumiSensorPresent == true){
    StaticJsonBuffer<200> jsonAddTempSensorBuffer;
    JsonObject& addTempSensorJson = jsonAddTempSensorBuffer.createObject();
    addTempSensorJson["name"] = chipIdF;
    addTempSensorJson["service"] = "TemperatureSensor";
    String addTempSensorString;
    addTempSensorJson.printTo(addTempSensorString);
    client.publish(addtopic,addTempSensorString);

    StaticJsonBuffer<200> jsonAddHumiSensorBuffer;
    JsonObject& addHumiSensorJson = jsonAddHumiSensorBuffer.createObject();
    addHumiSensorJson["name"] = chipIdG;
    addHumiSensorJson["service"] = "HumiditySensor";
    String addHumiSensorString;
    addHumiSensorJson.printTo(addHumiSensorString);
    client.publish(addtopic,addHumiSensorString);
  }
}

void maintAccessory(){
  //
}

void removeAccessory(){
  //
}

void setAccessory(){
    StaticJsonBuffer<200> setSwitchBuffer;
    JsonObject& setSwitchJson = setSwitchBuffer.createObject();
    
    if(accessoryName == std::string(chipIdA)){
      setSwitchJson["name"] = chipIdA;
      digitalSwitchAState = switchState;
      setSwitchJson["characteristic"] = "On";
      setSwitchJson["value"] = digitalSwitchAState;
      oldSwitchAState = digitalSwitchAState;
    }
    else if(accessoryName == std::string(chipIdB)){
      setSwitchJson["name"] = chipIdB;
      digitalSwitchBState = switchState;
      setSwitchJson["characteristic"] = "On";
      setSwitchJson["value"] = digitalSwitchBState;
      oldSwitchBState = digitalSwitchBState;
    }
    else if(accessoryName == std::string(chipIdC)){
      setSwitchJson["name"] = chipIdC;
      digitalSwitchCState = switchState;
      setSwitchJson["characteristic"] = "On";
      setSwitchJson["value"] = digitalSwitchCState;
      oldSwitchCState = digitalSwitchCState;
    }
    else if(accessoryName == std::string(chipIdD)){
      setSwitchJson["name"] = chipIdD;
      digitalSwitchDState = switchState;
      setSwitchJson["characteristic"] = "On";
      setSwitchJson["value"] = digitalSwitchDState;
      oldSwitchDState = digitalSwitchDState;
    }
    String setSwitchString;
    setSwitchJson.printTo(setSwitchString);
    client.publish(outtopic,setSwitchString);
  }

void getAccessory(){
    StaticJsonBuffer<200> getSwitchBuffer;
    JsonObject& getSwitchJson = getSwitchBuffer.createObject();
    getSwitchJson["name"] = accessoryName;
    getSwitchJson["characteristic"] = accessoryCharacteristic;
    if(accessoryName == std::string(chipIdA)){
    getSwitchJson["value"] = digitalSwitchAState;
    }else if(accessoryName == std::string(chipIdB)){
      getSwitchJson["value"] = digitalSwitchBState;
    }else if(accessoryName == std::string(chipIdC)){
      getSwitchJson["value"] = digitalSwitchCState;
    }else if(accessoryName == std::string(chipIdD)){
      getSwitchJson["value"] = currentSwitchDState;
    }else if(accessoryName == std::string(chipIdE)){
      getSwitchJson["value"] = contactSensorState;
    }else if(accessoryName == std::string(chipIdF)){
      getSwitchJson["value"] = celsiusTemp;
    }else if(accessoryName == std::string(chipIdG)){
      getSwitchJson["value"] = humidityTemp;
    }
    String getSwitchString;
    getSwitchJson.printTo(getSwitchString);
    client.publish(outtopic,getSwitchString);
}

void gettemperature() {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval) { 
    previousMillis = currentMillis;
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      celsiusTemp = 0;
      farenheitTemp = 0;
      humidityTemp = 0;
      }
      else{
        celsiusTemp = dht.computeHeatIndex(t, h, false);
        farenheitTemp = dht.computeHeatIndex(f, h);
        humidityTemp = h;
        Serial.print("Humidity: ");
        Serial.print(h);
        Serial.println();
        Serial.print("Temperature: ");
        Serial.print(t);
        Serial.print(" *C");
        Serial.println();
      }
  }
}

void publishtemperature(){
    StaticJsonBuffer<200> getTempBuffer;
    JsonObject& getTempJson = getTempBuffer.createObject();
    getTempJson["name"] = chipIdF;
    getTempJson["characteristic"] = "CurrentTemperature";
    getTempJson["value"] = celsiusTemp;
    String getTempString;
    getTempJson.printTo(getTempString);
    client.publish(outtopic,getTempString);

    StaticJsonBuffer<200> getHumiBuffer;
    JsonObject& getHumiJson = getHumiBuffer.createObject();
    getHumiJson["name"] = chipIdG;
    getHumiJson["characteristic"] = "CurrentRelativeHumidity";
    getHumiJson["value"] = humidityTemp;
    String getHumiString;
    getHumiJson.printTo(getHumiString);
    client.publish(outtopic,getHumiString);
    Serial.println("Publish Temperature");
}

void callback(const MQTT::Publish& pub){
  pub.topic().toCharArray(pubTopic,512);
  StaticJsonBuffer<512> jsoncallbackBuffer;
  JsonObject& mqttAccessory = jsoncallbackBuffer.parseObject(pub.payload_string());
  accessoryName = mqttAccessory["name"];
  if(mainttopic == std::string(pubTopic)){
    Serial.print("Maintenance");
    maintAccessory();
  }
  else if(intopic == std::string(pubTopic)){
    if (accessoryName == std::string(chipIdA) ||accessoryName == std::string(chipIdB) || accessoryName == std::string(chipIdC) || accessoryName == std::string(chipIdD)){
      accessoryCharacteristic = mqttAccessory["characteristic"];
      switchState = mqttAccessory["value"];
      setAccessory();
    }
  }
    else if (gettopic == std::string(pubTopic)){
      if (accessoryName == std::string(chipId) || accessoryName == std::string(chipIdA) || accessoryName == std::string(chipIdB) || accessoryName == std::string(chipIdC) || accessoryName == std::string(chipIdD) || accessoryName == std::string(chipIdE) || accessoryName == std::string(chipIdF) || accessoryName == std::string(chipIdG)){
        accessoryCharacteristic = mqttAccessory["characteristic"];
        getAccessory();
      }
  }
}
