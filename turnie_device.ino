#include <Arduino.h>
#include <LittleFS.h>
#include "Json_Handler.h"
#include "BLE_Manager.h"
#include "Display_Manager.h"

void setup() {
  Serial.begin(9600);
  LittleFS.begin(true);

  Display_Init(20);
  loadDisplayDataFromJson();

  BLE_Init();
}

void loop() {
  if (displayFlag == "text") {
      Display_FlowText(displayText.c_str());
  } else if (displayFlag == "image" && rgbData.size() > 0) {
      Display_ShowRGBRotCCW(rgbData.data(), rgbData.size(), 2000);
  }
  delay(100);
}
