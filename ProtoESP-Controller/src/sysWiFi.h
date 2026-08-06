#pragma once

//--------------------------------//web / wifi
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <ElegantOTA.h>

extern AsyncWebServer server(80);

void startWifiWeb();
