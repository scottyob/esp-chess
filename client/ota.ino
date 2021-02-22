#include "ota.h"

// Utility to extract header value from headers
String getHeaderValue(String header, String headerName) {
  return header.substring(strlen(headerName.c_str()));
}

/*
   Performs an over the air update.
*/
void OtaUpdater::update(String host, String filename) {
  long contentLength = 0;


  // Attempt to connect to webserver.
  if (!client.connect(host.c_str(), 80)) {
    Serial.println("Failed to connect to perform OTA.  Restarting");
    delay(1000);
    ESP.restart();
  }

  //Connection successful.  Sent HTTP GET for file
  String req = String("GET ") + filename + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Cache-Control: no-cache\r\n" +
               "Connection: close\r\n\r\n";
  Serial.println("Sending request");
  Serial.println(req);
  client.print(req);

  // Wait for server to start sending data.  If it takes it too long
  // assume something bad happened and just restart.
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println("Client Timeout !");
      client.stop();
      ESP.restart();
    }
  }

  while (client.available()) {
    // read line till /n
    String line = client.readStringUntil('\n');
    // remove space, to check if the line is end of headers
    line.trim();

    // if the the line is empty,
    // this is end of headers
    // break the while and feed the
    // remaining `client` to the
    // Update.writeStream();
    if (!line.length()) {
      //headers ended
      break; // and get the OTA started
    }

    // Check if the HTTP Response is 200
    // else break and Exit Update
    Serial.print("Response (line): ");
    Serial.println(line);
    if (line.startsWith("HTTP/1.1")) {
      if (line.indexOf("200") < 0) {
        Serial.println("Got a non 200 status code from server. Exiting OTA Update.");
        delay(1000);
        ESP.restart();
      }
    }

    // extract headers here
    // Start with content length
    if (line.startsWith("Content-Length: ")) {
      contentLength = atol((getHeaderValue(line, "Content-Length: ")).c_str());
      Serial.println("Got " + String(contentLength) + " bytes from server");
    }

    // Next, the content type
    if (line.startsWith("Content-Type: ")) {
      String contentType = getHeaderValue(line, "Content-Type: ");
      Serial.println("Got " + contentType + " payload.");
      if (contentType != "application/octet-stream") {
        Serial.println("Got " + String(contentType) + " content type.  Restarting");
        delay(1000);
        ESP.restart();
      }
    }
  }

  // Push the content to flash
  bool canBegin = Update.begin(contentLength);

  // If yes, begin
  if (canBegin) {
    Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
    // No activity would appear on the Serial monitor
    // So be patient. This may take 2 - 5mins to complete
    size_t written = Update.writeStream(client);

    if (written == contentLength) {
      Serial.println("Written : " + String(written) + " successfully");
    } else {
      Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". ERROR!" );
      delay(1000);
      ESP.restart();
    }

    if (Update.end()) {
      Serial.println("OTA done!");
      if (Update.isFinished()) {
        Serial.println("Update successfully completed. Rebooting.");
        ESP.restart();
      } else {
        Serial.println("Update not finished? Something went wrong!");
      }
    } else {
      Serial.println("Error Occurred. Error #: " + String(Update.getError()));
    }
  } else {
    // not enough space to begin OTA
    // Understand the partitions and
    // space availability
    Serial.println("Not enough space to begin OTA");
    client.flush();
  }
  // Annnnd reboot
  delay(1000);
  ESP.restart();

}
