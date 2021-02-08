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
*/


#define LED_PIN   15  // Pin number LED strip is on.

// If this is in "simple mode", we need to give the pins we expect inputs on.
// This matches up to the top left of the board.
const uint8_t simpleInputPins[][2] = {
  {16, 17},
  {19, 18},
};

Network network;
Table table(LED_PIN, true);
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
        display.update(network.getUrl(), "Acct Setup");
        break;
      default:
        display.update("", "Connected!");
    }
  }

  // Update the main table components
  table.update();
  network.update();
}
