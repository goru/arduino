#include <ESP8266WiFi.h>

#define WIFI_SSID   "<SSID>"
#define WIFI_PASSWD "<PASSWORD>"

#define TARGET_HOST "www.google.com"
#define TARGET_PORT 80

void setup() {
  Serial.begin(115200);
  
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.print("IP address is ");
  Serial.println(WiFi.localIP());

  WiFiClient client;
  Serial.print("Connecting to ");
  Serial.println(TARGET_HOST);
  
  if (!client.connect(TARGET_HOST, TARGET_PORT)) {
    Serial.print("Could not connect to ");
    Serial.println(TARGET_HOST);
    return;
  }

  client.print("GET / HTTP/1.0\r\n\r\n");

  delay(100);
  
  while (client.available()) {
    Serial.print(client.readStringUntil('\r'));
  }

  Serial.println();
  Serial.println("Done");
}

void loop() {
  // put your main code here, to run repeatedly:

}
