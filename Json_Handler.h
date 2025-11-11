#pragma once
#include <Arduino.h>
#include <vector>

extern String displayFlag;
extern String displayText;
extern std::vector<uint8_t> rgbData;

void loadDisplayDataFromJson();
void saveIncomingJson(const String& json);
void sendDataJson();
