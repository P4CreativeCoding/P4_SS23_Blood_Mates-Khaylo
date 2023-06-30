#include <BLEDevice.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define buttonPin 5
#define ledPin 13
int led_state = LOW;    // the current state of LED
int button_state;       // the current state of button
int last_button_state;

static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
boolean doConnect = false;
boolean connected = false;
BLEAdvertisedDevice* myDevice;
BLERemoteCharacteristic* pRemoteCharacteristic;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
        connected = true;
        Serial.println("Connected to server");
    }

    void onDisconnect(BLEClient* pclient) {
        connected = false;
        Serial.println("Disconnected from server");
    }
};

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
            BLEDevice::getScan()->stop();
            myDevice = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
        }
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Starting Arduino BLE Client application...");
    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    delay(1000);
    display.display();

    pinMode(buttonPin, INPUT_PULLUP); // set ESP32 pin to input pull-up mode
    pinMode(ledPin, OUTPUT);    
    button_state = digitalRead(buttonPin);
}

void displayStart() {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(0, 13);
    display.println("Hi!");
    display.setTextSize(1);
    display.setCursor(0, 33);
    display.println("Deine Spende wird    dringend gebraucht :(");
    display.display();
}

void loop() {
    if (doConnect) {
        BLEClient* pClient = BLEDevice::createClient();
        pClient->setClientCallbacks(new MyClientCallback());
        pClient->connect(myDevice);
        BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
        pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
        connected = true;
        doConnect = false;
    }

    if (connected) {
        uint32_t value = pRemoteCharacteristic->readUInt32();
        Serial.print("Received value: ");
        Serial.println(value);
        if (value == 1) {
          displayStart();
        }
    }
    last_button_state = button_state;      // save the last state
    button_state = digitalRead(buttonPin); // read new state

  if (last_button_state == HIGH && button_state == LOW) {
    Serial.println("The button is pressed");

    // toggle state of LED
    led_state = !led_state;

    // control LED arccoding to the toggled state
    digitalWrite(ledPin, led_state);

    delay(100);
  }
}

