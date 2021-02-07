#include <Wire.h>
#include "chessDisplay.h"
#include "table.h"

/*
 * ESP-Chess Board Client.
 * This sketch is to power the ESP32 powered chessboard.
 * Useful Links:
 *   Pinout:  https://www.studiopieters.nl/esp32-pinout/
 * 
 * Libraries Used:
 *   Adafruit_SSD1306
 *   Adafruit_NeoPixel
 */


#define LED_PIN   15  // Pin number LED strip is on.

// If this is in "simple mode", we need to give the pins we expect inputs on.
// This matches up to the top left of the board.
const uint8_t simpleInputPins[][2] = {
  {16, 17},
  {19, 18},
};

Table table(LED_PIN, true);
ChessDisplay display;

void setup() {
  Serial.begin(115200);

  //Initialize the I2C bus & Display
  Wire.begin();
  auto displaySuccess = display.begin();

  // Initialize the table memory & LED tests.
  Serial.println("Initializing Table");

  // TODO:  Re-Enable LED tests
  const auto runTests = false;//displaySuccess;
  table.begin(runTests, simpleInputPins);
  if (!displaySuccess)
    table.error();

}

void loop() {
  // put your main code here, to run repeatedly:
  table.update();
}
