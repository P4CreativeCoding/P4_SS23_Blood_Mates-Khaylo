#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
const int buttonPin = 14; // the number of the pushbutton pin
unsigned long buttonPressStartTime = 0;
const unsigned long minButtonPressDuration = 2000; // Minimum button press duration in milliseconds

#define SERVICE_UUID          "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

void setup() {
    Serial.begin(115200);

    pinMode(buttonPin, INPUT);

    BLEDevice::init("ESP32");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService* pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY
    );

    BLEDescriptor* pDescriptor = new BLEDescriptor((uint16_t)0x2901);
    pDescriptor->setValue("A very interesting variable");
    pCharacteristic->addDescriptor(pDescriptor);

    BLE2902* pBLE2902 = new BLE2902();
    pBLE2902->setNotifications(true);
    pCharacteristic->addDescriptor(pBLE2902);

    pService->start();

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);
    BLEDevice::startAdvertising();
    Serial.println("Waiting for a client connection...");
}

void loop() {
    if (deviceConnected) {
        int buttonState = digitalRead(buttonPin);

        // Check if the button is pressed and start the timer if not already started
        if (buttonState == HIGH && buttonPressStartTime == 0) {
            buttonPressStartTime = millis();
        }

        // Check if the button has been pressed for the minimum duration
        if (buttonPressStartTime != 0 && millis() - buttonPressStartTime >= minButtonPressDuration) {
            pCharacteristic->setValue(buttonState);
            pCharacteristic->notify();
            Serial.println(buttonState);
            buttonPressStartTime = 0; // Reset the button press start time
        }

        delay(100); // Add a small delay to debounce the button
    }

    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising();
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }

    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
}