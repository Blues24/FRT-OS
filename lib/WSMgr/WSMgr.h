#pragma once

#include <ESPAsyncWebServer.h>
#include "PinConfig.h"
#include "DriveMgr.h"
#include "ServoMgr.h"

void wsMgrBegin();
void handlePinsGet(AsyncWebServerRequest* request);
void handlePinsPost(AsyncWebServerRequest* request);