#ifndef TABLE_H
#define TABLE_H

#include "stdint.h"
#include <Adafruit_NeoPixel.h>

#define GRID_SIZE          8   // How hide/high is the table grid
#define SIMPLE_GRID_SIZE   2
#define GRID_LEDS          GRID_SIZE * GRID_SIZE
#define SIMPLE_GRID_LEDS   SIMPLE_GRID_SIZE * SIMPLE_GRID_SIZE
#define FILLED             true
#define EMPTY              false

/**
   Single square on grid.
*/
struct BoardLocation_T {
  uint8_t pinNumber;  // Input Pin number.
  uint8_t ledNumber;  // LED reference number
  bool    filled;     // If there is a piece on the location.
  uint8_t ledMode;    // Current mode to display on LED.
};

/**
   Represents the physical table.  Both the LED Array, and means
   of collecting updates.

   Supports *simple_mode*, that is, using a 2x2 grid directly connected
   to input ports on the ESP, otherwise will use I2C IO Expanders for full
   8x8 grid.
*/
class Table {
    BoardLocation_T board[GRID_SIZE][GRID_SIZE] = {{BoardLocation_T()}};
    bool simpleMode;

    Adafruit_NeoPixel pixels;
    void updatePieceLocations();
    void updateLed();
    void mirrorBoard();

  public:
    bool mirrorLocations;  // Should we mirror the locations of pieces on the board?  Good for testing/setup.

    Table(int led_pin, const bool simpleMode = false) : pixels(simpleMode ? SIMPLE_GRID_LEDS : GRID_LEDS, led_pin, NEO_GRB + NEO_KHZ800) {
      this->simpleMode = simpleMode;
      this->mirrorLocations = true;
    }
    // Initializes LED display, runs through tests
    void begin(const bool& runTest, const uint8_t simpleInputPins[][SIMPLE_GRID_SIZE]);
    void begin(const bool& runTest);
    // puts LED in an error state
    void error();
    // Update the table state
    void update();
};


#endif
