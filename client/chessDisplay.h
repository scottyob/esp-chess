#ifndef CHESS_DISPLAY
#define CHESS_DISPLAY

#include "stdint.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define CHESS_DISPLAY_OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels


/*
   Drives the chess board display.  This is based on
   the SSD1306 chip over I2C.  As there are a few
   different models of this going around, attempts to
   discover the address over I2C.
*/
class ChessDisplay {
  private:
    Adafruit_SSD1306 display;
  public:
    ChessDisplay() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, CHESS_DISPLAY_OLED_RESET) {}
    bool begin();
};

#endif
