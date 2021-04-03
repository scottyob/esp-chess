#ifndef NETWORK_H
#define NETWORK_H

#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include "table.h"
#include "ota.h"

// How many times to try and connect to WiFi
#define MAX_WIFI_ATTEMPTS  50

// Device name MUST marry up with the name defined in the AWS console
#define DEVICE_NAME "espchess-scott"

// From AWS IOT Things "Interact" wizard.
#define AWS_IOT_ENDPOINT "a37u0bfoinvf2q-ats.iot.us-west-2.amazonaws.com"

// MQTT topic that this device should puiblish to
#define AWS_IOT_TOPIC "$aws/things/" DEVICE_NAME "/tmp"
#define AWS_IOT_TOPIC "test"

// How many times we should attempt to connect to AWS
#define AWS_MAX_RECONNECT_TRIES 50

// How big of buffer space to create for sending JSON MQTT messages/responses
#define MESSAGE_LENGTH 1048

// how often to heartbeat in stats
#define REPORT_SECS 30

enum class WifiState {
  kIdle = 0,          // Not yet started to initialize the board.
  kInitializing,      // Initializing and attempting WiFi connect
  kWifiRequired,      // Waiting for WiFi credentials
  kInitializingCloud, // Connecting to the cloud service.
  kCertsRequired,     // Waiting for certificates to be uploaded
  kConnected,         // Connected to the service, looking good.
  kUpdating,          // Performing OTA update
};

enum class InternalWifiState {
  kIdle,              // Yet to attempt to connect to WiFi
  kConnecting,        // Attempting to connect to WiFi
  kSmartConfig,       // Attempting to get credentials from SmartConfig
  kConnected,         // WiFi is connected
};

enum class InternalMqttState {
  kIdle,              // Yet to attempt to MQTT broker
  kConnecting,
  kInvalid,
  kConnected,
};

class Network {
  private:
    // Certificate Information
    String deviceName, awsCertCa, awsCertPrivate, awsCertCrt, environment;
    void loadCert(); // Loads cert info from flash

    // Table
    Table* table;
    char jsonBuffer[MESSAGE_LENGTH];

    // Over the air update
    OtaUpdater updater;

    WifiState state;
    InternalWifiState wifiState;
    InternalMqttState mqttState;
    unsigned long lastState;
    WiFiClientSecure net;  // MQTT Secure Network
    MQTTClient client;  // MQTT client

    WebServer server;
    void attemptWifiConnect();
    void attemptSmartConfig();
    void beginMqtt();
    void startWebserver();
    void updateRemoteBoard();
    void updateDiagnostics();
    void messageReceived(String &topic, String &payload);  // MQTT message received
  public:
    Network(Table* tableRef) : server(80), table(tableRef), client(MESSAGE_LENGTH) {}
    void begin();
    void update();
    String getIp();        // URL to setup the certs
    WifiState getState() {
      return state;
    }
    bool missingConfig();
};

#endif
