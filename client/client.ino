#include <FS.h> //this needs to be first, or it all crashes and burns...
#include <Wire.h>
#include "SPIFFS.h"
#include "chessDisplay.h"
#include "table.h"
#include "network.h"
#include "chess.h"

/*
   ESP-Chess Board Client.
   This sketch is to power the ESP32 powered chessboard.
   Useful Links:
     Pinout:  https://www.studiopieters.nl/esp32-pinout/

   Libraries Used:
     Adafruit_SSD1306
     Adafruit_NeoPixel
     Qrcode
     EspFlash
     MQTT Library https://github.com/256dpi/arduino-mqtt - NOTE:  Using head for now as git submodule
     ArduinoJson (benoit Blanchon)
*/

#define VERSION   "1.1"
#define PROD_DOMAIN "chess.scottyob.com"
#define LED_PIN   15  // Pin number LED strip is on.

// If this is in "simple mode", we need to give the pins we expect inputs on.
// This matches up to the top left of the board.
const uint8_t simpleInputPins[][2] = {
  {16, 17},
  {19, 18},
};

Table table(LED_PIN);
Chess engine(&table);
Network network(&engine, &table);
ChessDisplay display;

void setup() {
  Serial.begin(115200);
  Serial.println("***************************************************");
  Serial.println("* ESP-Chess.  Software version " + String(VERSION));
  Serial.println("***************************************************");
  //Initialize internal flash memory, format on fail.
  randomSeed(analogRead(0));
  SPIFFS.begin(true);

  //Initialize the I2C bus & Display
  Wire.begin();
  auto displaySuccess = display.begin();

  // Initialize the table memory & LED tests.
  Serial.println("Initializing Table");

  // Initialize the network
  network.begin();

  // Run LED tests if display init succeeded & missing configs.
  auto runTests = network.missingConfig();
  if (!displaySuccess)
    runTests = false;

  const auto tableSuccess = table.begin(runTests, simpleInputPins);
  if (!displaySuccess) {
    Serial.println("ERROR:  LCD initialization failure");
    table.error();
  }

  if (!tableSuccess) {
    display.update("", "Diagnostics\nFailure");
    Serial.println("ERROR: Diagnostics failure");
    table.error();
  }

  network.onMessage(&messageCallback);
  engine.onMessage(&messageCallback);
}

void messageCallback(const String &qr, const String &message) {
  Serial.print("Displaying called back message: ");
  Serial.println(message);
  display.update(qr, message);
}

void loop() {
  static auto wifiState = WifiState::kIdle;
  auto newWifiState = network.getState();
  String url;

  if (newWifiState != wifiState) {
    // WiFi status has changed.  We need to update the display
    wifiState = newWifiState;

    switch (wifiState) {
      case WifiState::kInitializing:
        display.update("", "\n\nWifi\nConnecting");
        break;
      case WifiState::kWifiRequired:
        display.update("https://scottyob.github.io/esp-chess/wifi", "\n\nWiFi Setup\nRequired");
        break;
      case WifiState::kCertsRequired:
        // Build a URL
        url = String("https://");
        url += PROD_DOMAIN;
        url += "/setup/";
        url += network.getIp();
        Serial.print("Asking user to setup account: ");
        Serial.println(url);

        // display URL for setting up account
        display.update(url, "\n\nAcct Setup\nRequired");
        break;
      default:
        url = String("https://");
        url += PROD_DOMAIN;
        url += "/";
        display.update(url, "Connected!\n\nLoading\nGame");
        table.mirrorLocations = false;
    }
  }

  // Reset the board after 10 minutes of not being connected
  if (wifiState != WifiState::kConnected && millis() > 1000 * 60 * 10) {
    ESP.restart();
  }

  // Sleep the display if no activity for an extended period of time
  // (10 minutes)
  if (millis() - table.getLastActivity() > 60 * 1000 * 10) {
    display.off();
  } else {
    display.on();
  }

  // Update the main table components
  table.update();
  network.update();
  engine.loop();

  if (table.requiresUpdate && !table.mirrorLocations) {
    engine.didUpdate(false);
    table.requiresUpdate = false;
  }

}
