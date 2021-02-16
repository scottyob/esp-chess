#include "table.h"

int simpleLedLocations[][SIMPLE_GRID_SIZE] = {
  {1, 0},
  {2, 3},
};


void Table::begin(const bool& runTest) {
  this->begin(runTest, {});
}

void Table::begin(const bool& runTest, const uint8_t simpleInputPins[][SIMPLE_GRID_SIZE]) {
  // Setup and test the LED strip.
  Serial.println("Running LED Test");
  this->pixels.begin();
  if (runTest)
    led_test(this->pixels, this->simpleMode ? SIMPLE_GRID_LEDS : GRID_LEDS);

  if (sizeof(simpleInputPins) > 0) {
    // We're operating in simple mode.  Update the pin numbers

    for (int x = 0; x < SIMPLE_GRID_SIZE; x++)
      for (int y = 0; y < SIMPLE_GRID_SIZE; y++) {
        this->board[y][x].pinNumber = simpleInputPins[y][x];
        this->board[y][x].ledNumber = simpleLedLocations[y][x];
      }
  }
}

void Table::update() {
  this->updatePieceLocations();
  this->updateLed();
}

void Table::updatePieceLocations() {
  int upperBound = this->simpleMode ? SIMPLE_GRID_SIZE : GRID_SIZE;

  for (int x = 0; x < upperBound; x++) {
    for (int y = 0; y < upperBound; y++) {
      // TODO:  Handle non-simple mode I2C inputs.
      auto old = board[y][x].filled;
      board[y][x].filled = !digitalRead(board[y][x].pinNumber);
      if (board[y][x].filled != old)
        requiresUpdate = true;
    }
  }
}

void Table::mirrorBoard() {
  int upperBound = this->simpleMode ? SIMPLE_GRID_SIZE : GRID_SIZE;
  for (int x = 0; x < upperBound; x++)
    for (int y = 0; y < upperBound; y++)
      this->pixels.setPixelColor(this->board[y][x].ledNumber, 0, this->board[y][x].filled ? 255 : 0, 0);
  this->pixels.show();
}

void Table::updateLed() {
  if (this->mirrorLocations) {
    this->mirrorBoard();
    return;
  }
}

void Table::error() {
  while (1) {
    this->pixels.setPixelColor(0, 255, 0, 0);
    this->pixels.show();
    delay(500);
    this->pixels.setPixelColor(0, 0, 0, 0);
    this->pixels.show();
    delay(500);
  }
}

// Perform a quick test to cycle through each LED color.
void led_test(Adafruit_NeoPixel& p, const int& ledCount) {
  uint32_t testColors[] = {
    Adafruit_NeoPixel::Color(255, 0,   0),
    Adafruit_NeoPixel::Color(0,   255, 0),
    Adafruit_NeoPixel::Color(0,   0,   255),
    Adafruit_NeoPixel::Color(255, 255, 255),
    Adafruit_NeoPixel::Color(0,   0,   0)
  };

  for (int c = 0; c < sizeof(testColors) / sizeof(uint32_t); c++) {
    for (int i = 0; i < ledCount; i++)
      p.setPixelColor(i, testColors[c]);
    p.show();
    delay(750);
  }
}

// Gets a JSON state into buffer;
void Table::getJsonState(char* buffer, size_t bufferSize) {
  strcpy(buffer, "\"This is a demo of some string\"");
  static JSONBOARD_T doc;
  doc.clear();
  JsonArray rows = doc.to<JsonArray>();
  for (int y = 0; y < GRID_SIZE; y++) {
    JsonArray columns = rows.createNestedArray();
    for (int x = 0; x < GRID_SIZE; x++) {
      columns.add((int)board[y][x].filled);
    }
  }
  serializeJson(doc, buffer, bufferSize);
}
