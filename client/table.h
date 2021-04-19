#ifndef TABLE_H
#define TABLE_H

#include "stdint.h"
#include <Adafruit_NeoPixel.h>
#include "MCP23017.h"
#include <ArduinoJson.h>

#define GRID_SIZE          8   // How hide/high is the table grid
#define SIMPLE_GRID_SIZE   2
#define GRID_LEDS          GRID_SIZE * GRID_SIZE
#define SIMPLE_GRID_LEDS   SIMPLE_GRID_SIZE * SIMPLE_GRID_SIZE
#define FILLED             true
#define EMPTY              false
#define JSONBOARD_SIZE_T   2048

/**
   Single square on grid.
*/
struct BoardLocation_T {
  int pinNumber;  // Input Pin number.
  int ledNumber;  // LED reference number
  bool    filled;     // If there is a piece on the location.
  uint8_t ledMode;    // Current mode to display on LED.
};

/*
   A squares possible color.
*/
class BoardColor
{
  public:
    enum Value : uint8_t
    {
      NONE = 0,
      RED,
      GREEN,
      BLUE,
      ORANGE,
      LIGHTGREEN,
      GOLD,
      WHITISH,
      GRAY,
    };
    BoardColor() = default;
    BoardColor(uint8_t val) {
      value = (Value)val;
    }
    constexpr BoardColor(Value color) : value(color) { }
    operator Value() const {
      return value;  // case statements
    }
    uint32_t color() {
      switch (value) {
        case RED:
          return Adafruit_NeoPixel::Color(255,   0,   0);
        case GREEN:
          return Adafruit_NeoPixel::Color(0,   255,   0);
        case BLUE:
          return Adafruit_NeoPixel::Color(0,   0,   255);
        case ORANGE:
          return Adafruit_NeoPixel::Color(255,   165,   0);
        case LIGHTGREEN:
          return Adafruit_NeoPixel::Color(25,   25,   0);
        case GOLD:
          return Adafruit_NeoPixel::Color(249,   166,   2);
        case WHITISH:
          return Adafruit_NeoPixel::Color(200,   200,   200);
        case GRAY:
          return Adafruit_NeoPixel::Color(30,   30,   30);
        case NONE:
          return 0;
        default:
          return Adafruit_NeoPixel::Color(1,   2,   3);
      }
    }
  private:
    Value value;
};

/**
   Represents the physical table.  Both the LED Array, and means
   of collecting updates.

   Supports *simple_mode*, that is, using a 2x2 grid directly connected
   to input ports on the ESP, otherwise will use I2C IO Expanders for full
   8x8 grid.
*/
class Table {
    BoardLocation_T board[GRID_SIZE][GRID_SIZE] = {{BoardLocation_T()}};  // State of the board
    bool simpleMode;  // If we are in a simple debug mode
    // MCP IO Expanders
    MCP23017 mcp[4] = {
      MCP23017(0x20),
      MCP23017(0x21),
      MCP23017(0x22),
      MCP23017(0x23),
    };

    Adafruit_NeoPixel pixels;
    void updatePieceLocations();
    void updateLed();
    void mirrorBoard();
    int discoveredExpanders();

  public:
    bool mirrorLocations;  // Should we mirror the locations of pieces on the board?  Good for testing/setup.
    bool requiresUpdate;
    unsigned long lastActivity;

    Table(int led_pin) : pixels(SIMPLE_GRID_LEDS, led_pin, NEO_GRB + NEO_KHZ800) {
      this->mirrorLocations = true;
      requiresUpdate = false;
      lastActivity = millis();
    }
    // Initializes LED display, runs through tests
    bool begin(const bool& runTest, const uint8_t simpleInputPins[][SIMPLE_GRID_SIZE]);
    bool begin(const bool& runTest);
    void getJsonState(char* buffer, size_t bufferSize);
    // puts LED in an error state
    void error();
    // Update the table state
    void update();
    void render(const int doc[GRID_SIZE][GRID_SIZE], int brightness);
    unsigned long getLastActivity() {
      return lastActivity;
    }
};


#endif
