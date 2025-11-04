#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "Display_image.h"
#include "Display_Text.h"

// グローバル変数
String displayFlag = "text";    // 現在の表示タイプ
String displayText = "this is a test text";        // 表示するテキスト
std::vector<uint8_t> rgbData;   // 表示するRGB配列

// BLE設定 ----------------------------
#define SERVICE_UUID "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID_RX "abcd1234-5678-90ab-cdef-1234567890ab"
#define CHARACTERISTIC_UUID_TX "abcd1234-5678-90ab-cdef-1234567890ac"

BLECharacteristic *pTxCharacteristic;
BLECharacteristic *pRxCharacteristic;
bool deviceConnected = false;

// JSON読み込み
void loadDisplayDataFromJson() {
  File file = LittleFS.open("/data.json", "r");
  if (!file) {
    Serial.println("No data.json found");
    return;
  }

  StaticJsonDocument<4096> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.f_str());
    return;
  }

  JsonObject obj;

  // ① トップが配列かオブジェクトかを判定
  if (doc.is<JsonArray>()) {
    JsonArray arr = doc.as<JsonArray>();
    obj = arr[0];  // 最初の要素を使用
  } else if (doc.is<JsonObject>()) {
    obj = doc.as<JsonObject>();
  } else {
    Serial.println("Invalid JSON format");
    return;
  }

  // ② flag の取得
  if (!obj.containsKey("flag")) {
    Serial.println("No 'flag' key in JSON");
    return;
  }

  displayFlag = obj["flag"].as<String>();
  Serial.print("Display flag: ");
  Serial.println(displayFlag);

  // ③ テキスト表示
  if (displayFlag == "text") {
    if (obj.containsKey("text")) {
      displayText = obj["text"].as<String>();
      Serial.printf("Loaded text: %s\n", displayText.c_str());
    } else {
      Serial.println("No 'text' key found");
    }
  }

  // ④ 画像表示
  else if (displayFlag == "image" || displayFlag == "photo") {
    JsonArray arr = obj["rgb"].as<JsonArray>();
    rgbData.clear();
    for (JsonVariant v : arr) {
      rgbData.push_back(v.as<uint8_t>());
    }
    Serial.printf("Loaded image/photo data, length=%d\n", rgbData.size());

    if (obj.containsKey("brightness")) {
      int brightness = obj["brightness"].as<int>();
      Serial.printf("Brightness: %d\n", brightness);
      // Display_SetBrightness(brightness);
    }
  }
}


// BLE送信
void sendDataJson() {
  File file = LittleFS.open("/data.json", "r");
  if (!file) {
    Serial.println("No data.json found");
    return;
  }

  Serial.println("Sending /data.json contents...");
  while (file.available()) {
    String chunk = file.readStringUntil('\n');
    pTxCharacteristic->setValue(chunk.c_str());
    pTxCharacteristic->notify();
    delay(50);
  }
  file.close();
  Serial.println("Finished sending /data.json");
}

// BLEコールバック
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) override {
    deviceConnected = true;
    Serial.println("iOS device connected");
  }

  void onDisconnect(BLEServer *pServer) override {
    deviceConnected = false;
    Serial.println("iOS device disconnected");
    BLEDevice::startAdvertising();
  }
};

class WriteCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String value = pCharacteristic->getValue();
    if (value.length() == 0) return;

    Serial.print("Received: ");
    Serial.println(value);

    if (value == "GET_DATA") {
      sendDataJson();
      return;
    }

    // JSONとして解析
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, value);

    if (!error) {
      File file = LittleFS.open("/data.json", "w");
      if (file) {
        serializeJson(doc, file);
        file.close();
        Serial.println("Saved new JSON to /data.json");
        loadDisplayDataFromJson();  // ← 更新後に読み直す
      }
    }
  }
};

// デバイス名作成
String getUniqueName() {
  uint64_t mac = ESP.getEfuseMac();
  char macStr[13];
  sprintf(macStr, "%012llX", mac);
  return "ESP32_JSON_" + String(macStr).substring(6);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32 BLE JSON Receiver...");

  Display_Init(64);
  Matrix_SetTextBrightness(64);
  Matrix_Init();

  if (!LittleFS.begin(true)) {
    Serial.println("Failed to mount LittleFS");
    return;
  }

  Serial.println("LittleFS mounted");
  loadDisplayDataFromJson();

  // BLE初期化
  String deviceName = getUniqueName();
  BLEDevice::init("turnie_" + deviceName);

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE
  );
  pRxCharacteristic->setCallbacks(new WriteCallback());

  pTxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY
  );

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();

  Serial.println("BLE advertising started");
  Serial.println("Waiting for iOS connection...");
}

void loop() {
  // if (displayFlag == "text") {
    Text_Flow((char*)displayText.c_str());
  // }else if (displayFlag == "image" && rgbData.size() > 0) {
  //   Display_ShowRGBRotCCW(rgbData.data(), rgbData.size(), 2000);
  // }
  delay(100);
}
