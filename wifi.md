# WiFi

Your board is required to connect to WiFi to continue with the setup process.  

## Requirements & Background

The ESP32 MCU contains a 2.4Ghz radio.  Please make sure you join your mobile
device to a 2.4Ghz WiFi SSID you want the join the network.

We use [ESP SmartConfig](http://www.iotsharing.com/2017/05/how-to-use-smartconfig-on-esp32.html)
to enable the sharing of WiFi credentials with the ESP-Board.  When it first initializes, if it
is unable to connect to WiFi, it will direct you to this page to setup.

Please Note:  SmartConfig works by encoding your WiFi password into the length of the WiFi packet
of which the ESP-Chess board is listening and sniffing from the air.  This can be considered a
security risk as it effectively broadcasts your WiFI password, unencrypted.  We may make alternative
methods of allowing credentials at some point, but for now, this will do.

## 1.  Download.

For Android:  https://play.google.com/store/apps/details?id=com.khoazero123.iot_esptouch_demo&hl=en_US&gl=US
For iOS:  https://apps.apple.com/us/app/espressif-esptouch/id1071176700

## 2.  Enter in password

Enter in the password for your WiFi network.  Once entered, if the ESP-Chess board was able to discover
this, it should attempt to connect to the ESP-Chess cloud services.

## 3.  Copy Credentials

This page is eventually going to be dynamic and show generated certificates for your device.  You should
follow the new QR codes to copy this certificate to your device.
