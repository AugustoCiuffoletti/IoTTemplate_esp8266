#include <ESP8266WiFi.h>
#include "secret.h"

WiFiClient client;

int joinAP(int retry){
  char ipaddr[20];
  IPAddress ip;
  logline(0, 1, "Connecting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(MYSSID, MYPASSWD);
  while ( ( WiFi.status() != WL_CONNECTED ) && ((retry--) > 0) )
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if ( WiFi.status() != WL_CONNECTED ) {
    logline(0, 1, "Can't connect to AP");
    return 1;
  }
  ip=WiFi.localIP();
  sprintf(ipaddr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  logline(0, 2, "<T1> Connected as ", ipaddr);
  return 0;
}

void disconnectAP(){
  WiFi.disconnect();
  while (WiFi.status() == WL_CONNECTED) delay(1000);
}
