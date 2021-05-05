#include "network.h"
#include "WiFi.h"
#include "ESPFlashString.h"
#include <ArduinoJson.h>

#include <sstream>

// NOTE: A guide to AWS IOT I followed is https://savjee.be/2019/07/connect-esp32-to-aws-iot-with-arduino-code/

void Network::begin() {
  pinMode(0, INPUT); // On-Board button to reset all settings.
  loadCert(); // Load certs from flash

  // Setup the WiFi
  this->state = WifiState::kInitializing;
  this->wifiState = InternalWifiState::kConnecting;
  //Init WiFi as Station, attempt to connect to stored network
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin();

  this->lastState = millis();
}

void Network::update() {
  server.handleClient(); //Update webserver instances
  updateDiagnostics(); //Update any diagnostic instances

  // TODO:  Handle overflows (every 50 days)
  // Try to change a state every 500ms
  if ((this->mqttState != InternalMqttState::kConnected) && millis() - this->lastState < 500) {
    return;
  }
  this->lastState = millis();

  // on-board button for performing a "factory reset"
  if (!digitalRead(0)) {
    Serial.println("Resetting EVERYTHING!");
    ESPFlashString("/device_name").set("");
    ESPFlashString("/aws_cert_ca").set("");
    ESPFlashString("/aws_cert_crt").set("");
    ESPFlashString("/aws_cert_private").set("");
    ESPFlashString("/environment").set("");
    WiFi.disconnect(false, true);
    delay(500);
    ESP.restart();
  }

  // Attempt to update WiFi state if not connected
  switch (this->wifiState) {
    case InternalWifiState::kConnecting:
      this->attemptWifiConnect();
      return;
    case InternalWifiState::kSmartConfig:
      this->attemptSmartConfig();
      return;
  }

  // Attempt to update MQTT state if not connected
  switch (this->mqttState) {
    case InternalMqttState::kIdle:
      beginMqtt();
      return;
    case InternalMqttState::kConnected:
      if (!client.connected()) {
        Serial.println("MQTT No longer in connected state.  Restarting board");
        delay(1000);
        ESP.restart();
      }
      updateRemoteBoard();
      client.loop();
      return;
  }
}

/*
   Loads cert from flash into local strings
*/
void Network::loadCert() {
  Serial.println("Loading MQTT keys from flash storage");
  deviceName = ESPFlashString("/device_name").get();
  awsCertCa = ESPFlashString("/aws_cert_ca").get();
  awsCertCrt = ESPFlashString("/aws_cert_crt").get();
  awsCertPrivate = ESPFlashString("/aws_cert_private").get();
  environment = ESPFlashString("/environment").get();
  if (environment.length() == 0) {
    environment = "prod";
  }

  Serial.print("Missing populated configs: ");
  Serial.println(missingConfig() ? "True" : "False");
}

/*
   If any of the settings are blank, will flag this as missing
   config.
*/
bool Network::missingConfig() {
  return (
           deviceName == "" ||
           awsCertCa == "" ||
           awsCertCrt == "" ||
           awsCertPrivate == "" ||
           environment == "");
}

/*
   Sends diagnostics remotely and locally
*/
void Network::updateDiagnostics() {
  static unsigned long nextDue = 0;
  // Only update once every now and again if WiFi is connected
  if ((nextDue > millis()) || (wifiState != InternalWifiState::kConnected))
    return;
  nextDue = millis() + REPORT_SECS * 1000; //Report in again in 30 seconds.

  // Build JSON document for stats
  // See https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/Esp.h
  StaticJsonDocument<200> doc;
  doc["version"] = VERSION;
  doc["deviceName"] = deviceName;
  doc["uptime"] = millis();
  // Internal RAM
  doc["heapSize"] = ESP.getHeapSize(); //total heap size
  doc["freeHeap"] = ESP.getFreeHeap(); //available heap
  doc["minFreeHeap"] = ESP.getMinFreeHeap(); //lowest level of free heap since boot
  doc["maxAllocHeap"] = ESP.getMaxAllocHeap(); //largest block of heap that can be allocated at once
  //SPI RAM
  doc["psramSize"] = ESP.getPsramSize();
  doc["freePsram"] = ESP.getFreePsram();
  doc["minFreePsram"] = ESP.getMinFreePsram();
  doc["maxAllocPsram"] = ESP.getMaxAllocPsram();
  serializeJsonPretty(doc, jsonBuffer, MESSAGE_LENGTH);

  // Write stats to serial
  Serial.print("Running Stats: ");
  Serial.println(jsonBuffer);

  // Write stats to MQTT broker
  String topic = "update/stats/" + deviceName;
  client.publish(topic, jsonBuffer);
}

/*
   Begins the MQTT configuration.  If no settings found, asks operator
   to navigate to setup their MQTT configuration by using embedded
   webserver.
*/
void Network::beginMqtt() {
  if (
    (deviceName == "") ||
    (awsCertCa == "") ||
    (awsCertCrt == "") ||
    (awsCertPrivate == "")
  ) {
    // We do not have the appropriate keys.  Start configuring the device.
    startWebserver();
    return;
  }

  // Setup authentication using the stored keys in flash.
  net.setCACert(awsCertCa.c_str());
  net.setCertificate(awsCertCrt.c_str());
  net.setPrivateKey(awsCertPrivate.c_str());

  // Connect to the MQTT broker
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Try to connect to AWS and count how many times we retried.
  int retries = 0;
  Serial.print("Connecting to AWS IOT");

  // Connect several times over.  Consider moving this out to an update loop
  while (!client.connect(deviceName.c_str()) && retries < AWS_MAX_RECONNECT_TRIES) {
    Serial.print(".");
    delay(100);
    retries++;
  }

  // Make sure that we did indeed successfully connect to the MQTT broker
  // If not we just end the function and wait for the next loop.
  if (!client.connected()) {
    Serial.println(" Timeout!");
    startWebserver();
    return;
  }

  // If we land here, we have successfully connected to AWS!
  // And we can subscribe to topics and send messages.
  Serial.println("Connected!");
  mqttState = InternalMqttState::kConnected;

  // Subscribe to interesting topics, handle them
  client.subscribe(String("state/") + deviceName + "/#");
  client.onMessage([&](String & topic, String & payload) {
    this->messageReceived(topic, payload);
  });

  // Send the initial board state across.
  table->requiresUpdate = true;
  updateRemoteBoard();
}

// MQTT Message recieved.
void Network::messageReceived(String &topic, String &payload) {
  Serial.println("Received Message: ");
  Serial.println(payload);

  // Update last activity
  table->lastActivity = millis();

  //Turn payload into JSON document
  DynamicJsonDocument doc(MESSAGE_LENGTH);
  deserializeJson(doc, payload);

  // Run through the updates
  if (topic.endsWith("ota")) {
    Serial.println("OTA request recieved");
    if (doc["version"] == VERSION) {
      Serial.println("Ignoring OTA request.  Version matches");
      return;
    }
    Serial.println("Running OTA update routine");
    updateMessage("System\nUpdateing...\n\nPlease\nwait...");
    updater.update(doc["host"], doc["filename"]);
    return;
  } else if (topic.startsWith("state/")) {
    table->mirrorLocations = false;
    int values[GRID_SIZE][GRID_SIZE];
    if (doc["updateRequired"]) {
      // Remote side has requested we update the boards state
      table->requiresUpdate = true;
      return;
    }

    copyArray(doc["state"], values);

    table->render(values, doc["brightness"] || 255);
    if (doc["mirror"])
      table->mirrorLocations = true;
    updateMessage(doc["message"]);
    return;
  }

  Serial.println("Unhandled topic");
  Serial.println(String("topic: ") + topic);
  Serial.println(payload);
}

/*
   When we cannot connect to the MQTT broker, we will start a webserver
   and re-direct the operator to re-enter the credentials.
*/
void Network::startWebserver() {
  Serial.println("Configuring the device to enter Setup mode.  Starting Webserver");
  // Update the states
  mqttState = InternalMqttState::kInvalid;
  state = WifiState::kCertsRequired;

  // Handle webserver root page
  server.on("/", [&]() {
    // Redirect to Cert page
    server.sendHeader("Location", "https://scottyob.github.io/esp-chess/", true);
    server.send(302, "text/plain", "");
  });

  // Handle a status page
  server.on("/status", [&]() {
    ESPFlashString device_name("/device_name");
    ESPFlashString aws_cert_ca("/aws_cert_ca");
    ESPFlashString aws_cert_crt("/aws_cert_crt");
    ESPFlashString aws_cert_private("/aws_cert_private");    // Generate a debugging/status page
    String body = "";
    body += "Uptime: ";
    body += millis();
    body += "\n\n";

    body += "Device Name: " + device_name.get() + "\n\n";
    body += "AWS Cert CA: " + aws_cert_ca.get() + "\n\n";
    body += "AWS Cert CRT: " + aws_cert_crt.get() + "\n\n";
    body += "AWS Cert Private Key: " + aws_cert_private.get() + "\n\n";

    body += "\nWebserver Arguments: ";
    body += server.args();
    body += "\n";

    for (uint8_t i = 0; i < server.args(); i++) {
      body += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }

    server.send(200, "text/plain", body);
  });

  // Handle setting up the device
  server.on("/setup", [&]() {
    ESPFlashString device_name("/device_name");
    ESPFlashString aws_cert_ca("/aws_cert_ca");
    ESPFlashString aws_cert_crt("/aws_cert_crt");
    ESPFlashString aws_cert_private("/aws_cert_private");
    for (uint8_t i = 0; i < server.args(); i++) {
      auto key = server.argName(i);
      auto value = server.arg(i);

      if (key == "device_name") {
        device_name.set(value);
      } else if (key == "aws_cert_ca") {
        aws_cert_ca.set(value);
      } else if (key == "aws_cert_crt") {
        aws_cert_crt.set(value);
      } else if (key == "aws_cert_private") {
        aws_cert_private.set(value);
      }
    }
    server.send(200, "text/plain", "Device updated.  Will reboot.  You may now close this window");
    delay(1000);
    ESP.restart();
  });

  server.begin();
}

void Network::attemptWifiConnect() {
  static int attempts;
  static bool attemptedSmartConfig = false;

  // Check if chip was able to connect to WiFi.
  if (WiFi.status() == WL_CONNECTED) {
    attempts = 0;
    this->wifiState = InternalWifiState::kConnected;
    this->state = WifiState::kInitializingCloud;
    mqttState = InternalMqttState::kIdle;
    return;
  }

  // Check how many times we've attempted to connect to WiFi.
  Serial.print(".");
  attempts++;
  if (attempts > MAX_WIFI_ATTEMPTS) {
    // Give up with our current credentials
    attempts = 0;
    Serial.println("x");
    Serial.println("Exhausted WiFi connection attempts.");
    if (attemptedSmartConfig) {
      Serial.println("Already attempted SmartConfig once.  Rebooting");
      delay(5000);  // Wait 5 seconds, reboot.
      ESP.restart();
    }

    /* If there's a single piece set on the board at origin
       assume we don't want SmartConfig and setup an SSID instead
       using the portal mode.
    */
    if (table->isPortalSetupMode()) {
      String password = "";
      for (int i = 0; i < 8; i++) {
        int c = random(int('z') - int('a')) + int('a');
        password += String(char(c));
      }

      this->state = WifiState::kWifiPortalRequired;
      String qr = String("WIFI:S:ESP Chess;T:WPA;P:") + password + String(";;");
      updateMessage(qr, "\nNetwork:\nESP Chess\n\nPassword:\n" + password);
      ESP_wifiManager.startConfigPortal("ESP Chess", password.c_str());
      return;
    }

    // Setup SmartConfig to get new credentials.
    Serial.println("Attempting to setup SmartConfig");
    attemptedSmartConfig = true;
    this->wifiState = InternalWifiState::kSmartConfig;
    this->state = WifiState::kWifiRequired;
    WiFi.beginSmartConfig();
  }
}

void Network::updateRemoteBoard() {
  // Only send an MQTT update if one is required.
  if (!table->requiresUpdate) {
    return;
  }

  // MQTT topic to update
  String topic = "update/board/" + environment + "/" + deviceName;

  table->getJsonState(jsonBuffer, MESSAGE_LENGTH);

  // Serialize the board state to JSON
  //serializeJson(table->getJsonState(), jsonBuffer);

  // Debug some information.  Show the data and topic that was published
  Serial.print("Publishing to topic: ");
  Serial.println(topic);
  Serial.print("Data: ");
  Serial.println(jsonBuffer);

  // Publish the message to AWS
  client.publish(topic, jsonBuffer);

  // Flag update as being sent.
  table->requiresUpdate = false;

}

void Network::attemptSmartConfig() {
  // Check if we're still waiting for SmartConfig
  if (!WiFi.smartConfigDone())
    return;

  // SmartConfig successfully completed.  Attempt re-connect
  WiFi.stopSmartConfig();
  this->wifiState = InternalWifiState::kConnecting;
  this->state = WifiState::kInitializing;
}

String Network::getIp() {
  return WiFi.localIP().toString();
}

/*
   Runs the callback, if set to display a new status
   message.
*/
void Network::updateMessage(const String& qr, const String& message) {
  if (!messageCallback || !message)
    return;
  Serial.print("Running with callback message: ");
  Serial.println(message);
  messageCallback(qr, message);

}
