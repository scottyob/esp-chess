#include <FS.h> //this needs to be first, or it all crashes and burns...
#include <Wire.h>
#include "SPIFFS.h"
#include "chessDisplay.h"
#include "table.h"
#include "network.h"

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
     MQTT Library https://github.com/256dpi/arduino-mqtt
     ArduinoJson (benoit Blanchon)
*/

#define VERSION   0.1
#define PROD_DOMAIN "prod.d3rwxyk20q1igr.amplifyapp.com"
#define LED_PIN   15  // Pin number LED strip is on.

// If this is in "simple mode", we need to give the pins we expect inputs on.
// This matches up to the top left of the board.
const uint8_t simpleInputPins[][2] = {
  {16, 17},
  {19, 18},
};

Table table(LED_PIN, true);
Network network(&table);
ChessDisplay display;

void setup() {
  Serial.begin(115200);

  //Initialize internal flash memory, format on fail.
  SPIFFS.begin(true);

  //Initialize the I2C bus & Display
  Wire.begin();
  auto displaySuccess = display.begin();

  // Initialize the table memory & LED tests.
  Serial.println("Initializing Table");

  const auto runTests = displaySuccess;
  table.begin(runTests, simpleInputPins);
  if (!displaySuccess)
    table.error();

  // Initialize the network
  network.begin();

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
        display.update("", "Wifi Connecting");
        break;
      case WifiState::kWifiRequired:
        display.update("https://scottyob.github.io/esp-chess/wifi", "WiFi Setup\nRequired");
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
        display.update(url, "Acct Setup");
        break;
      default:
        display.update("", "Connected!");
    }
  }

  // Update the main table components
  table.update();
  network.update();
}
