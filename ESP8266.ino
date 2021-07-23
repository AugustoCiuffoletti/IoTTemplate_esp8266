/*
 * IMPORTANT: After rpogramming the device, short the A0 and RST pins (of a Wemos board)
 * to enable the wake-up at the end of the deep-sleep period
 * To unlock the key the operator has to create a corresponding key-value pair in the database. Refer to the
 * kviotApp to this purpose.
 */

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
#include <Base64.h>

#define VERSIONE "0.0"
// #define SERVER "192.168.113.181"
#define SERVER "10.0.113.1"
#define OP_CHECK        // Comment htis for automatic key-value pair creation (insecure)
#define PAYLOAD_LEN 70  // Recompute StaticJsonDocument size if you change this
#define KEY_LEN 8       // as above
#define BUFFER_LEN 300  // Better estimate need. HTTP request body size

// #define MASTERKEY "12345678"  // Comment this to have attended operation with operator acknowledgement

HTTPClient https;

// Structure associated to a key
struct softState{
  bool flag = false;        // encryption flag
//  char payload[PAYLOAD_LEN+1] = "{'id': 'xxxxxxxxxxxxxxxxxx', 'tref': 18, 'age': 7}";     // payload
  char payload[PAYLOAD_LEN+1] = "eyJpZCI6ICJQb3N0byAzIiwgInRyZWYiOiAxOCwgImFnZSI6IDB9"; // payload base64
  char nextKey[KEY_LEN+1];  // next key
} s;

int updateState(String input) {
// From https://arduinojson.org/v6/assistant/
  StaticJsonDocument<192> doc;
  DeserializationError error = deserializeJson(doc, input);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return 1;
  }
  s.flag = doc[0];                // false
  strncpy(s.payload,doc[1],sizeof(s.payload));       // {"id": "provaX", "tref": 18, "age": 7}"
  strncpy(s.nextKey,doc[2],sizeof(s.nextKey));    // "3cb9f8b7"
//  Serial.println("Deserialized");
  return 0;
}

int newKey() {
  char endpoint[400];
  char msgbuffer[BUFFER_LEN]; // better estimate needed
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(10*1E3);    // set timeout to 10 seconds
  Serial.println("=== PUT a new key in DB");
  // generate a new key
  unsigned long int nkey = random(ULONG_MAX);
//  nkey=0xd6a9a651;  // test only
  sprintf(s.nextKey,"%08x",nkey);
  Serial.print("New key is "); Serial.println(s.nextKey);
  // build URL for the new key
  sprintf(endpoint,"https://%s/%s",SERVER,s.nextKey);
  Serial.println(endpoint);
  #ifdef OP_CHECK  // This is the secure protocol under operator control
  // Check the generated key is available
  if (https.begin(client, endpoint)) {
    int x = https.GET();
    if(x == 200) {
      Serial.println("Duplicated key: try another");
      updateState(https.getString());
      https.end();
      Serial.println("===");
      return 1;
    } else {
      Serial.print("Available key is: ");Serial.println(s.nextKey);
      Serial.println("Waiting for OK from operator");
      https.end();
      Serial.println("===");
      return 0;
    }
  }
  #else           // This is the autonomous protocol
  // load the new key-value pair
  if (https.begin(client, endpoint)) {
    StaticJsonDocument<192> doc;
    doc.add(s.flag);
    doc.add(s.payload);
    doc.add(s.nextKey);
    serializeJson(doc, msgbuffer);
    https.addHeader("Content-Type", "application/json");
    Serial.print(s.nextKey); Serial.print(" -> "); Serial.println(msgbuffer);
    int x = https.PUT(msgbuffer);
    if(x == 200) {
      Serial.println("Success");
      https.end();
      Serial.println("===");
      return 0;
    } else {
      Serial.println("Problem creating a new key-value pair. HTTP error code " + String(x));
      https.end();
      Serial.println("===");
      return x;
    }
  }
  #endif OP_CHECK
  https.end();
  return NULL;
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
  char msgbuffer[BUFFER_LEN];
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(10*1E3);    // set timeout to 10 seconds
  Serial.println("=== POST a value to DB");
  sprintf(endpoint,"https://%s/%s",SERVER,s.nextKey);
  Serial.println(endpoint);
  if (https.begin(client, endpoint)) { 
    StaticJsonDocument<192> doc;
    doc.add(s.flag);
    doc.add(s.payload);
    doc.add(s.nextKey);
    serializeJson(doc, msgbuffer);
    Serial.print(s.nextKey); Serial.print(" -> "); Serial.println(msgbuffer);
    https.addHeader("Content-Type", "application/json");
    int x = https.POST(msgbuffer);
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

int businessLogic() {
  //Serial.println(Base64.decodedLength(s.payload, strlen(s.payload)));
  char payload[300];
  if ( s.flag ) {
    Base64.decode(payload, s.payload, strlen(s.payload));
  } else {
    sprintf(payload, "%s", s.payload);
  }
  //payload[50]=0x0;
  Serial.print("Decoded string is: "); Serial.print("-"); Serial.print(payload); Serial.println("-");
    
  StaticJsonDocument<96> doc_in;
  DeserializationError error = deserializeJson(doc_in, payload);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return 1;
  }
  char id[21];
  strncpy(id, doc_in["id"], strlen(id)); // "xxxxxxxxxxxxxxxx"
  int tref = doc_in["tref"]; // 18
  int age = doc_in["age"]; // 0
/*
 * Business logic goes here
 */
  age++; 
// End of business logic
  StaticJsonDocument<96> doc_out;
  doc_out["id"]=id;
  doc_out["tref"] = 18;
  doc_out["age"] = age;
  serializeJson(doc_out, payload);
  
  Base64.encode(s.payload, payload, strlen(payload));
  s.flag=true;
  return 0;
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600); // To bluetooth device
  Serial.print("\n========\nDeep-sleep recovery demo: V");
  Serial.println(VERSIONE);
  Serial.print("========\n");
  
  Serial.print("Access the WiFi AP");
  if ( joinAP(100) ) ESP.deepSleep(10*1E6);

  Serial.println(ESP.getResetReason());
  if ( resetInfo.reason == REASON_DEEP_SLEEP_AWAKE ) {
    Serial.println("Recovering state from deep sleep");
    ESP.rtcUserMemoryRead(0, (uint32_t*) &s.nextKey, sizeof(s.nextKey));
  } else {
#ifdef MASTERKEY
    Serial.println("=== Restart from master key");
    sprintf(s.nextKey,"%s",MASTERKEY);
#else
   Serial.println("=== Restart with key generation");
   while ( newKey() == 409 ) {
      Serial.println("Existing key, try another");
    }
#endif // MASTERKEY
  }
}

void loop() {
  while ( getValue() ) {
    if ( resetInfo.reason != REASON_DEEP_SLEEP_AWAKE ) {
      Serial1.print("*");Serial1.print(s.nextKey);      // send key value to Bluetooth transmitter
    }
    Serial.println("GET failed");
    delay(5000);
  }

  Serial.print("Next key is "); Serial.println(s.nextKey);
  businessLogic();
  postValue();
  disconnectAP();
  Serial.print("\n==== SLEEPING ====\n\n");
  
  ESP.rtcUserMemoryWrite(0, (uint32_t*) &s.nextKey, sizeof(s.nextKey));
  
  ESP.deepSleep(10*1E6);
//  delay(10000);
}
