#ifndef NETWORK_H
#define NETWORK_H

#include <WebServer.h>

#define MAX_WIFI_ATTEMPTS  50 // How many times to try and connect to WiFi


enum class WifiState {
  kIdle = 0,          // Not yet started to initialize the board.
  kInitializing,      // Initializing and attempting WiFi connect
  kWifiRequired,      // Waiting for WiFi credentials
  kInitializingCloud, // Connecting to the cloud service.
  kCertsRequired,     // Waiting for certificates to be uploaded
  kConnected,         // Connected to the service, looking good.
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
    WifiState state;
    InternalWifiState wifiState;
    InternalMqttState mqttState;
    unsigned long lastState;
    WebServer server;
    void attemptWifiConnect();
    void attemptSmartConfig();
    void beginMqtt();
    void startWebserver();
  public:
    Network() : server(80) {}
    void begin();
    void update();
    String getUrl();        // URL to setup the certs
    WifiState getState() {
      return state;
    }
};

#endif
