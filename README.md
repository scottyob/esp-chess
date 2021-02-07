# esp-chess

## About

This project allows a physical chess board powered by an ESP32 microcontroller
to connect to WiFi and play other users of ESP-Chessboards.  You will find
all of the design files, server and client components in this repository.

This is very much a work under construction, and is not usable in the current
state.

Server Goals:
 - Allow users to sign up, authenticate themselves
 - Allow users to generate certificates for their board
 - MQTT broker, allow users to create a match

Client:
 - Initialize all components, Display, IO expanders, LED Strip
 - Display QR Code and game status on displays
 - Connect to WiFi, allow user to enter credentials on app
 - Create web server, allow user to upload certificates
 - Join MQTT Server, send and receive board status
