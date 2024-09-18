#include <ESP8266WiFi.h>
#include <DHT.h>
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <FuzzyRule.h>
#include <FuzzyComposition.h>
#include <Fuzzy.h>
#include <FuzzyRuleConsequent.h>
#include <FuzzyOutput.h>
#include <FuzzyInput.h>
#include <FuzzyIO.h>
#include <FuzzySet.h>
#include <FuzzyRuleAntecedent.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define FIREBASE_HOST "https://multsmartiot-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_KEY "AIzaSyBHvL9k9c27LD4JXxBe9vxdwC8GA4Syr9w"
#define WIFI_SSID "shafira"
#define WIFI_PASSWORD "278278278"

#define DHT_PIN 4
#define DHT_TYPE DHT11

DHT dht(DHT_PIN,DHT_TYPE);
FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;

// for test
unsigned long sendDataPrevMillis = 0;
bool signupOk = false;

// fuzzy init
Fuzzy* fuzzy = new Fuzzy();
float outputpwm;
  

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(10);
  initWifi();

  config.api_key = FIREBASE_KEY;
  config.database_url = FIREBASE_HOST;

  // sign up right here 
  if(Firebase.signUp(&config,&auth,"","")){
    Serial.println("ok");
    signupOk = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config,&auth);
  Firebase.reconnectWiFi(true);

  // set a status connection
   if(Firebase.RTDB.setBool(&firebaseData,"Humtemp/status",true)){
      Serial.println("Passed");
      Serial.println("Path : " + firebaseData.dataPath());
      Serial.println("Type : " + firebaseData.dataType());
    } else {
      Serial.println("Failed");
      Serial.println("Reason :" + firebaseData.errorReason());
    } 

    // input fuzzy
  FuzzyInput* suhu = new FuzzyInput(1);
  FuzzySet* dingin = new FuzzySet(0,24,25,26);
  suhu->addFuzzySet(dingin);
  FuzzySet* sejuk = new FuzzySet(25,26,27,28);
  suhu->addFuzzySet(sejuk);
  FuzzySet* normal = new FuzzySet(27,28,29,30);
  suhu->addFuzzySet(normal);
  FuzzySet* panas = new FuzzySet(29,30,31,32);
  suhu->addFuzzySet(panas);
  FuzzySet* sangatpanas = new FuzzySet(31,32,33,34);
  suhu->addFuzzySet(sangatpanas);
  fuzzy->addFuzzyInput(suhu);

  // output 
  FuzzyOutput* pwm = new FuzzyOutput(1);
  FuzzySet* dinginOut = new FuzzySet(0,24,25,26);
  pwm->addFuzzySet(dinginOut);
  FuzzySet* sejukOut = new FuzzySet(25,26,27,28);
  pwm->addFuzzySet(sejukOut);
  FuzzySet* normalOut = new FuzzySet(27,28,29,30);
  pwm->addFuzzySet(normalOut);
  FuzzySet* panasOut = new FuzzySet(29,30,31,32);
  pwm->addFuzzySet(panasOut);
  FuzzySet* sangatPanasOut = new FuzzySet(31,32,33,34);
  pwm->addFuzzySet(sangatPanasOut);
  fuzzy->addFuzzyOutput(pwm);

  // rule
  // if(suhu == dingin) then pwnDingin
  FuzzyRuleAntecedent* ifSuhuDingin = new FuzzyRuleAntecedent();
  ifSuhuDingin->joinSingle(dingin);
  FuzzyRuleConsequent* thenPwnDingin = new FuzzyRuleConsequent();
  thenPwnDingin->addOutput(dinginOut);
  FuzzyRule* fuzzyRuleDingin = new FuzzyRule(1,ifSuhuDingin,thenPwnDingin);
  fuzzy->addFuzzyRule(fuzzyRuleDingin);

  // if(suhu == sejuk) then pwnSejuk
  FuzzyRuleAntecedent* ifSuhuSejuk = new FuzzyRuleAntecedent();
  ifSuhuSejuk->joinSingle(sejuk);
  FuzzyRuleConsequent* thenPwnSejuk = new FuzzyRuleConsequent();
  thenPwnSejuk->addOutput(sejukOut);
  FuzzyRule* fuzzyRuleSejuk = new FuzzyRule(2,ifSuhuSejuk,thenPwnSejuk);
  fuzzy->addFuzzyRule(fuzzyRuleSejuk);

  // if(suhu == normal) then pwnNormal
  FuzzyRuleAntecedent* ifSuhuNormal = new FuzzyRuleAntecedent();
  ifSuhuNormal->joinSingle(normal);
  FuzzyRuleConsequent* thenPwnNormal = new FuzzyRuleConsequent();
  thenPwnNormal->addOutput(normalOut);
  FuzzyRule* fuzzyRuleNormal = new FuzzyRule(3,ifSuhuNormal,thenPwnNormal);
  fuzzy->addFuzzyRule(fuzzyRuleNormal);

  // if(suhu == panas) then pwnPanas
  FuzzyRuleAntecedent* ifSuhuPanas = new FuzzyRuleAntecedent();
  ifSuhuPanas->joinSingle(panas);
  FuzzyRuleConsequent* thenPwnPanas = new FuzzyRuleConsequent();
  thenPwnPanas->addOutput(panasOut);
  FuzzyRule* fuzzyRulePanas = new FuzzyRule(4,ifSuhuPanas,thenPwnPanas);
  fuzzy->addFuzzyRule(fuzzyRulePanas);

   // if(suhu == sangatPanas) then pwnSangatPanas
  FuzzyRuleAntecedent* ifSuhuSangatPanas = new FuzzyRuleAntecedent();
  ifSuhuSangatPanas->joinSingle(sangatpanas);
  FuzzyRuleConsequent* thenPwnSangatPanas = new FuzzyRuleConsequent();
  thenPwnSangatPanas->addOutput(sangatPanasOut);
  FuzzyRule* fuzzyRuleSangatPanas = new FuzzyRule(5,ifSuhuSangatPanas,thenPwnSangatPanas);
  fuzzy->addFuzzyRule(fuzzyRuleSangatPanas);

}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  // output
  outputFuzzy(temperature);

  if(isnan(humidity) || isnan(temperature)){
    Serial.println("Fail to read a sensor");
    delay(500);
    return;
  }
 
    if(Firebase.RTDB.setFloat(&firebaseData,"Humtemp/humidity",humidity)){
      Serial.println("Passed");
      Serial.println("Path : " + firebaseData.dataPath());
      Serial.println("Type : " + firebaseData.dataType());
    } else {
      Serial.println("Failed");
      Serial.println("Reason :" + firebaseData.errorReason());
    } 

     if(Firebase.RTDB.setFloat(&firebaseData,"Humtemp/temperature",temperature)){
      Serial.println("Passed");
      Serial.println("Path : " + firebaseData.dataPath());
      Serial.println("Type : " + firebaseData.dataType());
    } else {
      Serial.println("Failed");
      Serial.println("Reason :" + firebaseData.errorReason());
    } 


  
}

void initWifi(){
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.println("Connecting to Wifi");
  }
  Serial.println("Connected to Wifi");
}

void outputFuzzy(float temperature){
  String status;
  
   // output pwm
  fuzzy->setInput(1,temperature);
  fuzzy->fuzzify();
  outputpwm = fuzzy->defuzzify(1);

  if(outputpwm == 0 || outputpwm <= 26){
    status = "Dingin";
  } else if(outputpwm == 25 || outputpwm <= 28){
    status = "Sejuk";
  } else if(outputpwm == 27 || outputpwm <= 30){
    status = "Normal";
  } else if(outputpwm == 29 || outputpwm <= 32){
    status = "Panas";
  } else if(outputpwm == 31 || outputpwm <= 34){
    status = "Sangat Panas";
  } else {
    status = "Tidak Terdeteksi";
  }

   if(Firebase.RTDB.setString(&firebaseData,"Humtemp/output",status)){
      Serial.println("Passed");
      Serial.println("Path : " + firebaseData.dataPath());
      Serial.println("Type : " + firebaseData.dataType());
    } else {
      Serial.println("Failed");
      Serial.println("Reason :" + firebaseData.errorReason());
    } 
}

