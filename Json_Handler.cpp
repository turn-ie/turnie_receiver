#include "Json_Handler.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

String displayFlag;
String displayText;
std::vector<uint8_t> rgbData;

void loadDisplayDataFromJson() {
    File file = LittleFS.open("/data.json", "r");
    if (!file) {
        Serial.println("No data.json found");
        return;
    }

    StaticJsonDocument<4096> doc;
    if (deserializeJson(doc, file)) {
        Serial.println("JSON parse error");
        file.close();
        return;
    }
    file.close();

    JsonVariant root = doc.as<JsonVariant>();

    JsonObject obj;

    if (root.is<JsonArray>()) {
        obj = root.as<JsonArray>()[0].as<JsonObject>();
    } else if (root.is<JsonObject>()) {
        obj = root.as<JsonObject>();
    } else {
        Serial.println("Invalid JSON");
        return;
    }

    displayFlag = obj["flag"].as<String>();

    if (displayFlag == "text") {
        displayText = obj["text"].as<String>();
    } else {
        rgbData.clear();
        for (auto v : obj["rgb"].as<JsonArray>()) {
            rgbData.push_back(v.as<uint8_t>());
        }
    }
}

void saveIncomingJson(const String& json) {
    File file = LittleFS.open("/data.json", "w");
    if (!file) {
        Serial.println("Failed to write JSON file");
        return;
    }
    file.print(json);
    file.close();
}
