#ifndef OTA_H
#define OTA_H

#include <WiFi.h>
#include <Update.h>

/*
   Over the Air (OTA) update module
   Performs a HTTP over the air update.  This should become a lot simpler once
   Future versions of the ESP lib come out, and for now, this does NOT support
   HTTPS :'(
*/

class OtaUpdater {
  private:
    WiFiClient client;

  public:
    void update(String host, String filename);
};

#endif
