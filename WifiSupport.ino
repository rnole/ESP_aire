#define NTP_SERVER_NAME   "south-america.pool.ntp.org"
#define NTP_PACKET_SIZE   48
#define UDP_PORT        2390      

IPAddress timeServerIP;
WiFiUDP udp;

bool shouldSaveConfig = false;
char static_ip[16] = "192.168.43.36";
char static_gw[16] = "192.168.43.1";
char static_sn[16] = "255.255.255.0";
byte packetBuffer[NTP_PACKET_SIZE];


//callback notifying us of the need to save config
void saveConfigCallback(void)
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void Wifi_get_network_info(void){

  if(SPIFFS.begin()){
    File configFile = SPIFFS.open("/config.json", "r");
    if(configFile)
    {
      size_t size = configFile.size();

      std::unique_ptr<char[]> buf(new char[size]);

      configFile.readBytes(buf.get(), size);
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      json.printTo(Serial);
      if(json.success())
      {
        strcpy(static_ip, json["ip"]);
        strcpy(static_gw, json["gateway"]);
        strcpy(static_sn, json["subnet"]);
      }
    }
        
  }
}

void Wifi_store_network_info(void){
   if(shouldSaveConfig){
    
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();

    json["ip"] = WiFi.localIP().toString();
    json["gateway"] = WiFi.gatewayIP().toString();
    json["subnet"] = WiFi.subnetMask().toString();

    File configFile = SPIFFS.open("/config.json", "w");

    json.prettyPrintTo(Serial);
    json.printTo(configFile);
    configFile.close();
  }
}

void Wifi_init_v2(void){
   // -- Initializing the configuration.
  iotWebConf.init();

  client.setFingerprint(fingerprint);

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  Serial.println("Starting UDP");
  udp.begin(UDP_PORT);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  
}


void Wifi_init_without_FS(void){

  const char* ssid     = "richard";
  const char* password = "20091990";
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  udp.begin(UDP_PORT);
  Serial.print("Local port: ");
  Serial.println(udp.localPort()); 
}



uint32_t Wifi_get_time_stamp(void){

  WiFi.hostByName(NTP_SERVER_NAME, timeServerIP); 
  Send_ntp_packet(timeServerIP);
  delay(1000);

  int cb = udp.parsePacket();
  if(cb)
  {
    Serial.print("packet received, length=");
    Serial.println(cb);
    udp.read(packetBuffer, NTP_PACKET_SIZE);
   
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);
    
    const unsigned long seventyYears = 2208988800UL;
    unsigned long epoch = secsSince1900 - seventyYears;

    //substract 18000 seconds to get to GMT-5
    epoch = epoch - 18000UL;
    return epoch;
  }
  else
  {
    Serial.println("no packet yet");
    return 0;
  }

}

uint32_t Send_ntp_packet(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;            // Stratum, or type of clock
  packetBuffer[2] = 6;            // Polling Interval
  packetBuffer[3] = 0xEC;         // Peer Clock Precision
                                  // 8 bytes of zero for Root Delay 
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  udp.beginPacket(address, 123);
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();  
}


void Wait_until_connected_to_network(void){

  uint32_t timestamp = Wifi_get_time_stamp();
  
  while (timestamp == 0){
    timestamp = Wifi_get_time_stamp();
    iotWebConf.doLoop();
  }
  
}
