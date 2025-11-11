#include "BLE_Manager.h"
#include "Json_Handler.h"
#include <BLEDevice.h>
#include <BLEServer.h>

#define SERVICE_UUID "12345678-1234-1234-1234-1234567890ab"
#define RX_UUID "abcd1234-5678-90ab-cdef-1234567890ab"
#define TX_UUID "abcd1234-5678-90ab-cdef-1234567890ac"

BLECharacteristic *pTxCharacteristic;

class WriteCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {
    String rxValue = c->getValue();
    if (rxValue.isEmpty()) return;

    static String buffer = "";
    buffer += rxValue;

    if (buffer.endsWith("}")) {
      saveIncomingJson(buffer);
      loadDisplayDataFromJson();
      buffer = "";
    }
  }
};

void BLE_Init() {
  BLEDevice::init("turnie_device");

  BLEServer *server = BLEDevice::createServer();
  BLEService *service = server->createService(SERVICE_UUID);

  auto rx = service->createCharacteristic(
      RX_UUID, BLECharacteristic::PROPERTY_WRITE);
  rx->setCallbacks(new WriteCallback());

  pTxCharacteristic = service->createCharacteristic(
      TX_UUID, BLECharacteristic::PROPERTY_NOTIFY);

  service->start();

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->start();
}
