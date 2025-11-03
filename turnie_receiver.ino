#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <LittleFS.h>

#define SERVICE_UUID "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID_RX "abcd1234-5678-90ab-cdef-1234567890ab" // 受信用 (iOS→ESP32)
#define CHARACTERISTIC_UUID_TX "abcd1234-5678-90ab-cdef-1234567890ac" // 送信用 (ESP32→iOS)

BLECharacteristic *pTxCharacteristic;
BLECharacteristic *pRxCharacteristic;

bool deviceConnected = false;

void sendDataJson() {
  File file = LittleFS.open("/data.json", "r");
  if (!file) {
    Serial.println("No data.json found");
    return;
  }

  Serial.println("Sending /data.json contents...");

  while (file.available()) {
    String chunk = file.readStringUntil('\n'); // 1行ごとに送信（必要なら改行で分割）
    pTxCharacteristic->setValue(chunk.c_str());
    pTxCharacteristic->notify();
    delay(50); // BLE通知は少し間隔を空ける
  }

  file.close();
  Serial.println("Finished sending /data.json");
}

// サーバーコールバック（接続状態管理）
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    Serial.println("iOS device connected");
  }

  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("iOS device disconnected");
    // 再度アドバタイズを開始
    BLEDevice::startAdvertising();
  }
};

// 書き込みコールバック
class WriteCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String value = pCharacteristic->getValue();
    if (value.length() == 0) return;

    Serial.print("Received: ");
    Serial.println(value);

    if (value == "GET_DATA") {
      sendDataJson(); // JSON送信
      return;
    }

    if (!LittleFS.exists("/data.json")) {
      File newFile = LittleFS.open("/data.json", "w");
      if (newFile) {
        newFile.close();
        Serial.println("Created new /data.json");
      } else {
        Serial.println("Failed to create /data.json");
        return;
      }
    }
    File file = LittleFS.open("/data.json", "a");
    if (!file) {
      Serial.println("Failed to open /data.json for appending");
      return;
    }
    file.println(value);
    file.close();
    Serial.println("Appended to /data.json");
  }
};

String getUniqueName() {
  uint64_t mac = ESP.getEfuseMac();  // 48bit MAC
  char macStr[13];
  sprintf(macStr, "%012llX", mac);   // 例: 24A5B7E9C123
  String name = "ESP32_JSON_" + String(macStr).substring(6); // 下位3バイトを使用
  return name;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32 BLE JSON Receiver...");

  // LittleFSを初期化（setup時に1回だけ）
  if (!LittleFS.begin(true)) {
    Serial.println("Failed to mount LittleFS");
    return;
  }
  Serial.println("LittleFS mounted");

  // BLE初期化
  String deviceName = getUniqueName();
  BLEDevice::init("turnie_"+deviceName);

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // iOS → ESP32 (WRITE)
  pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE
  );
  pRxCharacteristic->setCallbacks(new WriteCallback());

  // ESP32 → iOS (NOTIFY)
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
}