# esp-chess

## About

This project allows a physical chess board powered by an ESP32 microcontroller
to connect to WiFi and play other users of ESP-Chessboards.  You will find
all of the design files, server and client components in this repository.

To build a board, please check out the ESP-Chess-Board repo https://github.com/Tayherb/ESP-Chess-Board/

## ESPChess Setup

The Arduino IDE is what we've used to develop, but does not yet have a good library editor, so use these instructions to manage:

File->Preferences
Additional boards URL:  https://dl.espressif.com/dl/package_esp32_index.json

Tools->Board Manager
Search for "esp32" then install (used version 1.06)
Set board->ESP Dev Module

Libraries Used
- QRCode by Richard Moore https://github.com/ricmoo/qrcode/ (used lib 0.0.1)
- Adafruit SSD1306 by Adafruit https://github.com/adafruit/Adafruit_SSD1306 (used 2.5.1)
- Adafruit NeoPixel by Adafruit https://github.com/adafruit/Adafruit_NeoPixel (used 1.10.4)
- MCP2301 by Bertrand Lemasle https://github.com/blemasle/arduino-mcp23017 (used 2.0)
- ArduinoJson by Benoit Blanchon https://arduinojson.org/?utm_source=meta&utm_medium=library.properties (used 6.19.3)
- ESPFlash by Dale Giancono https://github.com/DaleGia/ESPFlash (used ???)
- ESP WifiManager by Khoi Hoang https://github.com/khoih-prog/ESP_WiFiManager (used 1.10.1)
- MQTT by Joel Gaehwiler https://github.com/256dpi/arduino-mqtt (used 2.5.0)
