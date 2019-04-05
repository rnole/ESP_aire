
int Http_post_request(char *dataBuffer){

  if (!client.connect(HOST_NAME, HOST_PORT)) 
  {
   Serial.println("connection failed");
   return 0;
  }

  //pubSubClient.loop();

  int result = 0;

  //Para HTTPS, descomentar esto 
  if (client.verify(fingerprint, HOST_NAME)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }
  
  Serial.println(URL);
  
  client.println(String("POST ") + URL + " HTTP/1.1");
  client.print("Host: ");
  client.println(HOST_NAME);
  client.print("Content-Length: ");
  client.println(strlen(dataBuffer));
  client.println("Content-Type: application/json");
  client.println();
  client.println(dataBuffer);

  Serial.println("request sent");

  int lineIndex = 0;
  
  while (client.connected()) {
    //pubSubClient.loop();
    
    line = client.readStringUntil('\n');
    Serial.print("line index: ");
    Serial.print(lineIndex);
    Serial.print(" Line: ");
    Serial.println(line);

    if(line.indexOf("200") > 0)
    {
      Serial.println("Server response was OK");
      result = 1;
    }

    lineIndex++;
    yield();
    
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }

    if(lineIndex > 10)
    {
      break;
    }
  }
  
  //String line = client.readString();
  Serial.println("reply was:");
  Serial.println("==========");
  //Serial.println(line);
  Serial.println("==========");
  Serial.println("closing connection");
  //pubSubClient.loop();
  delay(3000);

  return result;
}
