#include <ESP8266WiFi.h>
#include "secret.h"

WiFiClient client;

int joinAP(int retry){
  Serial.print("\nConnecting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(MYSSID, MYPASSWD);
  while ( ( WiFi.status() != WL_CONNECTED ) && ((retry--) > 0) )
  {
    delay(500);
    Serial.print(".");
  }
  if ( WiFi.status() != WL_CONNECTED ) {
    Serial.println("Can't connect to AP");
    return 1;
  }
  Serial.println();
  Serial.print("Connected as ");
  Serial.println(WiFi.localIP());
  digitalWrite(LED_BUILTIN,LOW);
  return 0;
}

int disconnectAP(){
  WiFi.disconnect();
  while (WiFi.status() == WL_CONNECTED) delay(1000);
  digitalWrite(LED_BUILTIN,HIGH);
}
