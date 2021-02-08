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
  if (!this->display.begin(SSD1306_SWITCHCAPVCC, address))
    return false;

  // Display initialized successfully
  this->display.display();
  return true;;
}

void ChessDisplay::update(String url, String message) {
  uint8_t qrcodeData[qrcode_getBufferSize(3)];

  display.clearDisplay();  // Clear display buffer
  messageCanvas.fillScreen(SSD1306_BLACK);

  if (url == "") {
    // Draw logo bitmap for left hand side
    display.drawBitmap(0, 0, LOGO, 64, 64, SSD1306_WHITE);
  } else {
    // Generate a QR code and write to left hand side of display.
    // Blow it up to 2x the size.
    qrcode_initText(&qrcode, qrcodeData, 3, 0, url.c_str());
    for (uint8_t x = 0; x < qrcode.size; x++)
      for (uint8_t y = 0; y < qrcode.size; y++)
        if (qrcode_getModule(&qrcode, x, y))
          for (int i1 = 0; i1 < 2; i1++)
            for (int i2 = 0; i2 < 2; i2++)
              display.drawPixel(x * 2 + i1, y * 2 + i2, SSD1306_WHITE);
  }

  messageCanvas.setTextSize(1);
  messageCanvas.setTextColor(SSD1306_WHITE);
  messageCanvas.setCursor(0, 0);
  messageCanvas.cp437(true);
  messageCanvas.write(message.c_str());

  // Update the canvas to the right of the display.
  for (int x = 0; x < 64; x++) {
    for (int y = 0; y < 64; y++) {
      if (messageCanvas.getPixel(x, y)) {
        display.drawPixel(x + 64, y, SSD1306_WHITE);
      }
    }
  }

  this->display.display();
}
