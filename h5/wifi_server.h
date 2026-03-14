#pragma once

#include "globals.h"
#include <WebServer.h>
#include <WebSocketsServer.h>

// Buffer helpers (defined here, used by gcode.cpp and wifi_server.cpp)
bool bufferAvailable(CircleBuffer* b);
bool writeBuffer(CircleBuffer* b, char c);
bool writeBuffer(CircleBuffer* b, const char* str);
bool writeBuffer(CircleBuffer* b, const String& str);
bool writeBuffer(CircleBuffer* b, float f, int precision);
bool writeBuffer(CircleBuffer* b, long value);
char shiftBuffer(CircleBuffer* b);
void initBuffer(CircleBuffer* b, size_t size);
void clearBuffer(CircleBuffer* b);

// WiFi status
void setWiFiStatus(const String& status);

// Web request handlers
void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
void handleClientRequests();
void handleGcodeAdd();
void handleGcodeList();
void handleGcodeGet();
void handleGcodeRemove();
void handleStatus();

// FreeRTOS task
void taskWiFi(void* param);
