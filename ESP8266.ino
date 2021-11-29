// include the library code:
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

ESP8266WebServer server;
char* ssid = "ssid";
char* password = "password";
void setup() {
  WiFi.begin(ssid,password);
  Serial.begin(9600);
  while(WiFi.status()!=WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/",handleIndex);
  server.begin();
}

void loop() {
  server.handleClient();

}

void handleIndex()
{
  // Send a JSON-formatted request with key "type" and value "request"
  // then parse the JSON-formatted response with keys "gas" and "distance"
  DynamicJsonDocument doc(1024);
  String tagID = "";
  String access= "";
  // Sending the request
  doc["type"] = "request";
  serializeJson(doc,Serial);
  // Reading the response
  boolean messageReady = false;
  String message = "";
  while(messageReady == false) { // blocking but that's ok
    if(Serial.available()) {
      message = Serial.readString();
      messageReady = true;
    }
  }
  // Attempt to deserialize the JSON-formatted message
  DeserializationError error = deserializeJson(doc,message);
  if(error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  tagID = (const char*)doc["UID"];
  tagID = doc["UID"].as<const char*>();
  tagID = doc["UID"].as<String>();
  access = (const char*)doc["Access"];
  access = doc["Access"].as<const char*>();
  access = doc["Access"].as<String>();
  // Prepare the data for serving it over HTTP
  String output = "UID: " + tagID + "\n";
  output += "Access: " + access;
  // Serve the data as plain text, for example
  server.send(200,"text/plain",output);
}
