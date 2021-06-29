#include "limits.h"            // random key generation
#include "wifi.h"

extern "C" {
  #include "user_interface.h"
  /* see https://github.com/esp8266/Arduino/blob/master/tools/sdk/include/user_interface.h for rst_resaon codes */
  extern struct rst_info resetInfo;
}
#include <ESP8266HTTPClient.h> // https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient
#include <WiFiClientSecureBearSSL.h> // https://arduino-esp8266.readthedocs.io/en/laMONGODB/esp8266wifi/bearssl-client-secure-class.html
#include <ArduinoJson.h>

#define VERSIONE "0.0"
#define SERVER "192.168.113.181"
HTTPClient https;

// Structure associated to a key
struct softState{
  bool flag = false;        // flag
  char msg[80] = "{'id': 'provaX', 'tref': 18, 'age': 7}";     // payload
  char nextKey[9];  // next key
} s;



int updateState(String input) {
// From https://arduinojson.org/v6/assistant/
//  Serial.println(input);
  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, input);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return 1;
  }
  s.flag = doc[0];                // false
  strncpy(s.msg,doc[1],80);       // "{'id': 'provaX', 'tref': 18, 'age': 7}"
  strncpy(s.nextKey,doc[2],8);    // "3cb9f8b7"
//  Serial.println("Deserialized");
  return 0;
}



int newKey(char key[]) {
  char endpoint[400];
  char payload[80];
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(10*1E3);    // set timeout to 10 seconds
  Serial.println("=== PUT a new key in DB");
  sprintf(endpoint,"https://%s/%s",SERVER,key);
  Serial.println(endpoint);
  if (https.begin(client, endpoint)) {
    StaticJsonDocument<128> doc;
    strncpy(s.nextKey,key,9);
    doc.add(s.flag);
    doc.add(s.msg);
    doc.add(s.nextKey);
    serializeJson(doc, payload);
    https.addHeader("Content-Type", "application/json");
    Serial.print(key); Serial.print(" -> "); Serial.println(payload);
    int x = https.PUT(payload);
    if(x == 200) {
      Serial.println("Success");
      https.end();
      Serial.println("===");
      return 0;
    } else {
      Serial.println("Problem creating a new key-value pair. HTTP error code " + String(x));
      https.end();
      Serial.println("===");
      return 1;
    }
  }
  https.end();
  return NULL;
}

int businessLogic() {
  return 0;
}

int getValue() {
  char endpoint[400];
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(10*1E3);    // set timeout to 10 seconds
  Serial.println("=== GET a value from DB");
  sprintf(endpoint,"https://%s/%s",SERVER,s.nextKey);
  Serial.println(endpoint);
  if (https.begin(client, endpoint)) {
    int x = https.GET();
    if(x == 200) {
      Serial.print(s.nextKey); Serial.print(" -> "); Serial.println(https.getString());
      updateState(https.getString());
      https.end();
      Serial.println("===");
      return 0;
    } else {
      Serial.println("Problem getting softState. HTTP error code " + String(x));
      https.end();
      Serial.println("===");
      return 1;
    }
  }
  https.end();
  return NULL;
}

int postValue() {
  char endpoint[400];
  char payload[80];
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(10*1E3);    // set timeout to 10 seconds
  Serial.println("=== POST a value to DB");
  sprintf(endpoint,"https://%s/%s",SERVER,s.nextKey);
  Serial.println(endpoint);
  if (https.begin(client, endpoint)) { 
    StaticJsonDocument<128> doc;
    doc.add(s.flag);
    doc.add(s.msg);
    doc.add(s.nextKey);
    serializeJson(doc, payload);
    Serial.print(s.nextKey); Serial.print(" -> "); Serial.println(payload);
    https.addHeader("Content-Type", "application/json");
    int x = https.POST(payload);
    if(x == 200) {
      Serial.println("Success");
      https.end();
      Serial.println("===");
      return 0;
    } else {
      Serial.println("Problem posting softState. HTTP error code " + String(x));
      https.end();
      Serial.println("===");
      return 1;
    }
  }
  https.end();
  return NULL;
}

void setup() {
  char key[9];
  Serial.begin(115200);
  Serial.print("\n========\nDeep-sleep recovery demo: V");
  Serial.println(VERSIONE);
  Serial.print("========\n");

  Serial.println(ESP.getResetReason());
  if ( resetInfo.reason == REASON_DEEP_SLEEP_AWAKE ) {
    Serial.println("Recovering state from deep sleep");
    ESP.rtcUserMemoryRead(0, (uint32_t*) &s, sizeof(s));
  } else {
    Serial.println("Restart with key generation"); Serial.println("=== Generating a new key");
    unsigned long int nkey = random(ULONG_MAX);
    sprintf(key,"%x",nkey);
    Serial.print("New key is "); Serial.println(key);
    Serial.println("===");
  
    Serial.print("Access the WiFi AP");
    if ( joinAP(100) ) ESP.deepSleep(10*1E6);
    Serial.print("Access successful\n");
    newKey(key);
    disconnectAP(); 
  }
}

void loop() {
  Serial.print("Access the WiFi AP");
  if ( joinAP(100) ) ESP.deepSleep(10*1E6);
  while ( getValue() ) {
    Serial.println("GET failed");
    delay(5000);
  }
  Serial.print("Next key is "); Serial.println(s.nextKey);
  businessLogic();
  postValue();
  disconnectAP();
  Serial.print("\n==== SLEEPING ====\n\n");

  ESP.rtcUserMemoryWrite(0, (uint32_t*) &s, sizeof(s));
  
  ESP.deepSleep(10*1E6);
//  delay(10000);

}
