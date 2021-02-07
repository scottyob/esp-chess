#include "chessDisplay.h"
#include <Wire.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

/**
   Discovers the I2C address of the display, assuming that IO expanders
   will be on addresses 0x20 to 0x23
*/
byte discover() {
  byte error, address;

  Serial.println("Scanning...");

  byte foundAddress = 0;
  for (address = 1; address < 127; address++ )
  {
    // Skip past IO expander addresses
    if (address >= 0x20 && address <= 0x23)
      continue;

    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("Found I2C device on ");
      Serial.println(address, HEX);
      if (foundAddress) {
        Serial.print("Expected only 1 I2C device for display.  Got ");
        Serial.print(foundAddress, HEX);
        Serial.print(", ");
        Serial.println(address, HEX);
        return 0;
      }
      foundAddress = address;
    }
    else if (error == 4)
    {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
      return 0;
    }
  }
  return foundAddress;
}

/**
   Initializes display.  Returns init success
*/
bool ChessDisplay::begin() {
  auto address = discover();

  // No I2C display found.
  if (!address)
    return false;

  // Initialize the display
  if(!this->display.begin(SSD1306_SWITCHCAPVCC, address))
    return false;

  // Display initialized successfully
  this->display.display();
  return true;;
}
