#include "network.h"
#include "WiFi.h"
#include "ESPFlashString.h"
#include <sstream>

void Network::begin() {
  this->state = WifiState::kInitializing;
  this->wifiState = InternalWifiState::kConnecting;
  //Init WiFi as Station, attempt to connect to stored network
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin();

  this->lastState = millis();
}

void Network::update() {
  server.handleClient();

  // TODO:  Handle overflows (every 50 days)
  // Try to change a state every 500ms
  if (millis() - this->lastState < 500) {
    return;
  }
  this->lastState = millis();

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
  }
}

/*
   Begins the MQTT configuration.  If no settings found, asks operator
   to navigate to setup their MQTT configuration by using embedded
   webserver.
*/
void Network::beginMqtt() {
  Serial.println("Loading keys from flash storage");
  ESPFlashString key("/key");
  if (key.get() == "") {
    //Unable to connect.  Ask the user to re-enter key
    startWebserver();
  }

  //TODO:  Connect to MQTT Broker given the keys.
  
}

/*
   When we cannot connect to the MQTT broker, we will start a webserver
   and re-direct the operator to re-enter the credentials.
*/
void Network::startWebserver() {
  Serial.println("Unable to connect to MQTT.  Starting webserver to replace creds");
  // Update the states
  mqttState = InternalMqttState::kInvalid;
  state = WifiState::kCertsRequired;

  // Handle webserver
  server.on("/", [&]() {
    // Redirect to Cert page
    server.sendHeader("Location", "https://scottyob.github.io/esp-chess/", true);
    server.send(302, "text/plain", "");
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

    // Setup SmartConfig to get new credentials.
    Serial.println("Attempting to setup SmartConfig");
    attemptedSmartConfig = true;
    this->wifiState = InternalWifiState::kSmartConfig;
    this->state = WifiState::kWifiRequired;
    WiFi.beginSmartConfig();
  }
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

String Network::getUrl() {
  String str = "http://" + WiFi.localIP().toString();
  str += "/";
  Serial.print("Generated URL: ");
  Serial.println(str);
  return str;
}
