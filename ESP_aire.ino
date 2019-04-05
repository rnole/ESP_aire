#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>
#include <SoftwareSerial.h>
#include <DNSServer.h>
#include <IotWebConf.h>
#include <ArduinoJson.h>
#include "Definitions.h"

char test_buffer[350] = {0};
String returnString;

// SHA1 fingerprint of the certificate
const char* fingerprint = "BB 37 30 98 AD 72 39 3F 0C 25 7E 49 E5 F9 85 CA 00 05 C7 AC";

//Use WiFiClientSecure class to create TLS connection
//WiFiClient client;
WiFiClientSecure client;
WiFiClient espClient;

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "ESP_AIRE_AP";

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "esp_aire_password";

DNSServer dnsServer;
WebServer server(80);

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);
String line;

uint32_t NO_value = 0;
uint32_t CCS_value = 0;
uint32_t CH4_value = 0;
uint32_t O3_value = 0;
uint32_t NO2_value = 0;
uint32_t SO2_value = 0;
uint16_t pm1_value = 0;
uint16_t pm25_value = 0;
uint16_t pm10_value = 0;
uint32_t CO2_value = 0;
float Temperature_value = 0;
float Humidity_value = 0;


void setup() {
  Serial.begin(9600);
  Wifi_init_v2();
  Serial.println("Waiting for network connection...");
  Wait_until_connected_to_network();
  Serial.println("Connection Established");
  Serial.flush();
}

void loop() {
  iotWebConf.doLoop();
  
  Get_measured_data();
  Http_post_request(test_buffer);

  uint32_t start_time = millis();
  while((millis() - start_time) < TIME_TO_WAIT_MS){
      iotWebConf.doLoop();
      delay(500); 
  }
}

void Get_measured_data(void){

  char buf_data[100]    = {0};

  StaticJsonBuffer<350> dataJSONBuffer;
  JsonObject& data = dataJSONBuffer.createObject();

  uint32_t timestamp = Wifi_get_time_stamp();
  Serial_read_data(buf_data);
  Serial_parse_data(buf_data);
  
  while (timestamp == 0){
    timestamp = Wifi_get_time_stamp();
    iotWebConf.doLoop();
  }
  
  data["id"]        = ESP_AIRE_ID;
  data["timestamp"] = timestamp;
  data["ch4"]       = 0;
  data["o3"]        = O3_value;
  data["no2"]       = NO2_value;
  data["so2"]       = SO2_value;
  data["co2"]       = CO2_value;
  data["temp"]      = Temperature_value;
  data["hum"]       = Humidity_value;
  data["no"]        = NO_value;
  data["voc"]       = CCS_value;
  data["pm1_0"]     = pm1_value;
  data["pm2_5"]     = pm25_value;
  data["pm10"]      = pm10_value;

  data.prettyPrintTo(Serial);
  data.prettyPrintTo(test_buffer, sizeof(test_buffer));

}

void Serial_read_data(char *buf){
  int serial_rx_flag = 0; 
  int idx = 0;

  while(serial_rx_flag == 0){
    iotWebConf.doLoop();
    
    while(Serial.available() > 0){
       buf[idx++] = (char)Serial.read();
       serial_rx_flag = 1;
       delay(10);
    }
  }
}


void Serial_parse_data(char *buf){

  char *p_cmd;

  p_cmd             = buf;
  NO_value          = atoi(p_cmd);
  p_cmd             = strstr(p_cmd, ",") + 1;
  CCS_value         = atoi(p_cmd);
  p_cmd             = strstr(p_cmd, ",") + 1;
  O3_value          = atoi(p_cmd);
  p_cmd             = strstr(p_cmd, ",") + 1;
  NO2_value         = atoi(p_cmd);
  p_cmd             = strstr(p_cmd, ",") + 1;
  SO2_value         = atoi(p_cmd);
  p_cmd             = strstr(p_cmd, ",") + 1;
  NO_value          = atoi(p_cmd);
  p_cmd             = strstr(p_cmd, ",") + 1;
  CO2_value         = atoi(p_cmd);
  p_cmd             = strstr(p_cmd, ",") + 1;
  pm1_value         = atof(p_cmd);
  p_cmd             = strstr(p_cmd, ",") + 1;
  pm25_value        = atof(p_cmd);
  p_cmd             = strstr(p_cmd, ",") + 1;
  pm10_value        = atof(p_cmd);
  p_cmd             = strstr(p_cmd, ",") + 1;
  Temperature_value = atof(p_cmd);
  p_cmd             = strstr(p_cmd, ",") + 1;
  Humidity_value    = atof(p_cmd);
}

/**
 * Handle web requests to "/" path.
 */
void handleRoot(void){
  
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>ECM AP - Home page</title></head><body>Hello!";
  s += "Go to <a href='config'>configure page</a> to change settings.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}
