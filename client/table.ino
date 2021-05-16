#include "table.h"

// For the purpose of simple mode, these are the
// chess board locations and pin numbers we expect
// to trigger them on.
int simpleLedLocations[][SIMPLE_GRID_SIZE] = {
  {1, 0},
  {2, 3},
};

int LED_LOCATIONS[][GRID_SIZE] = {
  { 0,  1,  2,  3,  4,  5,  6,  7},
  {15, 14, 13, 12, 11, 10,  9,  8},
  {16, 17, 18, 19, 20, 21, 22, 23},
  {31, 30, 29, 28, 27, 26, 25, 24},
  {32, 33, 34, 35, 36, 37, 38, 39},
  {47, 46, 45, 44, 43, 42, 41, 40},
  {48, 49, 50, 51, 52, 53, 54, 55},
  {63, 62, 61, 60, 59, 58, 57, 56},
};

// First byte is used for expander number, second for
// expander pin number.  Represents pin location to
// poll for chess board coordinate.
int PIN_LOCATIONS[][GRID_SIZE] = {
  {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F},
  {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07},
  {0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F},
  {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17},
  {0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F},
  {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27},
  {0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F},
  {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37},
};

#define WHITE 1
#define BLACK 0
const uint8_t IDLE_BRIGHTNESS[][GRID_SIZE] = {
  {WHITE, BLACK, WHITE, BLACK, WHITE, BLACK, WHITE, BLACK},
  {BLACK, WHITE, BLACK, WHITE, BLACK, WHITE, BLACK, WHITE},
  {WHITE, BLACK, WHITE, BLACK, WHITE, BLACK, WHITE, BLACK},
  {BLACK, WHITE, BLACK, WHITE, BLACK, WHITE, BLACK, WHITE},
  {WHITE, BLACK, WHITE, BLACK, WHITE, BLACK, WHITE, BLACK},
  {BLACK, WHITE, BLACK, WHITE, BLACK, WHITE, BLACK, WHITE},
  {WHITE, BLACK, WHITE, BLACK, WHITE, BLACK, WHITE, BLACK},
  {BLACK, WHITE, BLACK, WHITE, BLACK, WHITE, BLACK, WHITE},
};

int Table::discoveredExpanders() {
  int discovered = 0;

  byte error, address;
  for (address = 0x20; address < 0x24; address++ )
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Discovered IO Expander at: ");
      Serial.println(address);
      discovered++;
    }
  }
  return discovered;
}

bool Table::begin(const bool& runTest) {
  return this->begin(runTest, {});
}

bool Table::begin(const bool& runTest, const uint8_t simpleInputPins[][SIMPLE_GRID_SIZE]) {
  // Perform a simple diagnostics test to find the IO Expanders
  auto ioFound = discoveredExpanders();
  if (ioFound == 0) {
    this->simpleMode = true;
  } else if (ioFound != 4) {
    Serial.println("ERROR:  invalid amount of IO Expanders discovered");
    return false;
  } else {
    // We should be using the entire grid
    this->simpleMode = false;
    pixels.updateLength(GRID_LEDS);

    // Begin MCP IO expanders
    for (int i = 0; i < 4; i++) {
      mcp[i].init();
      //for (int pin = 0; pin < 16; pin++)
      //  mcp[i].pinMode(pin, INPUT);
    }
  }

  // Setup and test the LED strip.
  Serial.println("Running LED Test");
  this->pixels.begin();
  if (runTest)
    led_test(this->pixels, this->simpleMode ? SIMPLE_GRID_LEDS : GRID_LEDS);

  if (simpleMode && sizeof(simpleInputPins) > 0) {
    // We're operating in simple mode.  Update the pin numbers

    for (int x = 0; x < SIMPLE_GRID_SIZE; x++)
      for (int y = 0; y < SIMPLE_GRID_SIZE; y++) {
        this->board[y][x].pinNumber = simpleInputPins[y][x];
        this->board[y][x].ledNumber = simpleLedLocations[y][x];
      }
  } else {
    for (int x = 0; x < GRID_SIZE; x++)
      for (int y = 0; y < GRID_SIZE; y++) {
        this->board[y][x].pinNumber = PIN_LOCATIONS[y][x];
        this->board[y][x].ledNumber = LED_LOCATIONS[y][x];
      }
  }

  // Setup the initial board state.
  updatePieceLocations();
  requiresUpdate = false;

  return true;
}

void Table::update() {
  this->updatePieceLocations();
  this->updateLed();
}

void Table::updatePieceLocations() {
  int upperBound = this->simpleMode ? SIMPLE_GRID_SIZE : GRID_SIZE;
  bool changed = false;

  for (int x = 0; x < upperBound; x++) {
    for (int y = 0; y < upperBound; y++) {
      auto old = board[y][x].filled;

      if (simpleMode) {
        board[y][x].filled = !digitalRead(board[y][x].pinNumber);
      }
      else {
        // Extract IO expander number and pin number
        auto pinNumber = board[y][x].pinNumber;
        auto ioNum = pinNumber >> 4;
        auto ioPin = pinNumber & 0x0F;
        board[y][x].filled = !mcp[ioNum].digitalRead(ioPin);

      }

      if (board[y][x].filled != old) {
        changed = true;
        lastActivity = millis();
      }
    }
  }

  // For debugging, dump the board state to the console.
  if (changed) {
    this->requiresUpdate = true;
    Serial.println("New board status: ");
    for (int y = 0; y < GRID_SIZE; y++) {
      for (int x = 0; x < GRID_SIZE; x++) {
        Serial.print(board[y][x].filled);
      }
      Serial.println();
    }
    Serial.println();
  }


}

/*
   Mirrors the magnet state.
*/
void Table::mirrorBoard() {
  int upperBound = this->simpleMode ? SIMPLE_GRID_SIZE : GRID_SIZE;
  for (int x = 0; x < upperBound; x++)
    for (int y = 0; y < upperBound; y++) {
      /*
         Show the idle square gray color unless pieces are placed, in which case, override with
         green.
      */
      auto color = IDLE_BRIGHTNESS[y][x];
      if (this->board[y][x].filled) {
        color = 0;
      }
      this->pixels.setPixelColor(this->board[y][x].ledNumber, color, this->board[y][x].filled ? 32 : color, color);
    }
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

void colorWipe(Adafruit_NeoPixel & p, uint32_t color, int wait) {
  int grid_size = sqrt(p.numPixels());
  for(int y = 0; y < grid_size; y++) {
    p.clear();
    for(int x = 0; x < grid_size; x++) {
      p.setPixelColor(y * grid_size + x, color);
    }
    p.show();
    delay(wait);
  }
  for(int x = 0; x < grid_size; x++) {
    p.clear();
    for(int y = 0; y < grid_size; y++) {
      auto display_x = x;
      if(y % 2 == 1)
        display_x = grid_size - x - 1;
      
      p.setPixelColor(y * grid_size + display_x, color);
    }
    p.show();
    delay(wait);
  }
}

// Perform a quick test to cycle through each LED color.
void led_test(Adafruit_NeoPixel & p, const int& ledCount) {
  colorWipe(p, p.Color(255,   0,   0)     , 60); // Red
  colorWipe(p, p.Color(  0, 255,   0)     , 60); // Green
  colorWipe(p, p.Color(  0,   0, 255)     , 60); // Blue
  colorWipe(p, p.Color(255, 255, 255)     , 60); // White
}

void Table::render(const int doc[GRID_SIZE][GRID_SIZE], int brightness) {
  //static DynamicJsonDocument doc(JSONBOARD_SIZE_T);
  //deserializeJson(doc, json);
  for (int y = 0; y < GRID_SIZE; y++) {
    for (int x = 0; x < GRID_SIZE; x++) {
      uint8_t gridState = doc[y][x];
      auto color = BoardColor(gridState).color();
      if (!color)
        color = Adafruit_NeoPixel::Color(
                  IDLE_BRIGHTNESS[y][x],
                  IDLE_BRIGHTNESS[y][x],
                  IDLE_BRIGHTNESS[y][x]);
      pixels.setPixelColor(board[y][x].ledNumber, color);
    }
  }
  pixels.show();
}

// Gets a JSON state into buffer;
void Table::getJsonState(char* buffer, size_t bufferSize) {
  static DynamicJsonDocument doc(JSONBOARD_SIZE_T);
  doc.clear();
  doc["version"] = VERSION;
  
  for (int y = 0; y < GRID_SIZE; y++) {
    for (int x = 0; x < GRID_SIZE; x++) {
      doc["state"][y][x] = (int)board[y][x].filled;
    }
  }
  serializeJson(doc, buffer, bufferSize);
}

// Is the piece in top left (origin) only enabled
bool Table::isPortalSetupMode() {
  updatePieceLocations();
  if(!board[0][0].filled)
    return false;
  for(int y = 0; y < GRID_SIZE; y++) {
    for(int x = 0; x < GRID_SIZE; x++) {
      // Skip past origin
      if(!y && !x)
        continue;
      if(board[y][x].filled)
        return false;
    }
  }
  return true;
}

// Populates 64 square array with '1' or ' '
void Table::populateSquares(char* squares) {
  int pos = 0;
  for(int y = 0; y < GRID_SIZE; y++) {
    for(int x = 0; x < GRID_SIZE; x++) {
      squares[pos++] = board[y][x].filled ? '1' : ' ';
    }
  }
  squares[pos] = '\0';
}
