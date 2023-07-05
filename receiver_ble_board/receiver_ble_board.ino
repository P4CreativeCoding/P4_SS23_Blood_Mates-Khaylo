#include <BLEDevice.h>

// display libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// RFID-Reader Libraries
#include <SPI.h>
#include <MFRC522.h>

// RFID Pins
#define SS_PIN 26
#define RST_PIN 27

// pins
#define buttonPin 5
#define ledPin 13
#define ledPin_owner 12

int led_state = LOW;    // the current state of LED
int button_state = LOW; // the current state of button
int last_button_state;  // for saving last button state
bool reset = false;     // reset condition for completet action
bool donationIsNeeded = false;

// BLE specifivations
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
boolean doConnect = false;
boolean connected = false;
BLEAdvertisedDevice* myDevice;
BLERemoteCharacteristic* pRemoteCharacteristic;

// display specifications
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// RFID specifications
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;


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

    // BLE initialisation
    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);

    // display initialisation
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    delay(1000);
    display.display();

    pinMode(buttonPin, INPUT_PULLUP); // set ESP32 pin to input pull-up mode
    pinMode(ledPin, OUTPUT);    
    button_state = digitalRead(buttonPin);

    // initialise RFID reader
    SPI.begin();
    mfrc522.PCD_Init();
    Serial.println("Scan a MIFARE Classic card");

    pinMode(ledPin_owner, OUTPUT);

    for (byte i = 0; i < 6; i++) {
      key.keyByte[i] = 0xFF;
    }
}

void displayStart() {

  if (reset == true) {
    // display this message if all participants have confirmed donation
    Serial.println("reset start");
    display.clearDisplay();
    display.setTextSize(3);
    display.setCursor(0, 21);
    display.println("Danke!"); 
    donationIsNeeded = false;
    
    
  } else if (reset == false && donationIsNeeded == true) {
    // display this message if donations are needed
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(0, 13);
    display.println("Hi!");
    display.setTextSize(1);
    display.setCursor(0, 33);
    display.println("Deine Spende wird    dringend gebraucht :(");
  }

  display.display(); // Update the display with current data

  if (reset) {
    // reset everything after delay
    delay(7000);
    reset = false;
    digitalWrite(ledPin_owner, LOW);
    digitalWrite(ledPin, LOW);
    // display.clearDisplay();
   
  }
}

bool checkDonation(byte* blockData, byte blockSize) {
  // read data
  char donationString[] = "user has donated";
  int donationLength = sizeof(donationString) - 1; // Exclude null terminator

  if (blockSize < donationLength) {
    return false; // Block size is smaller than the donation string
  }

  for (int i = 0; i <= blockSize - donationLength; i++) {
    bool match = true;
    for (int j = 0; j < donationLength; j++) {
      if (blockData[i + j] != donationString[j]) { // if user has not donated
        match = false;
        break;
      }
    }
    if (match) {
      return true; // Donation string found in the block
    }
  }

  return false; // Donation string not found in the block
}

void loop() {
    // BLE connection
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
        uint32_t value = pRemoteCharacteristic->readUInt32(); // read received value
        Serial.print("Received value: ");
        Serial.println(value);
        if (value == 1) {
          donationIsNeeded = true; // change donation state
          displayStart(); // activate display
        }
    }

    last_button_state = button_state;      // save the last state
    button_state = digitalRead(buttonPin); // read new state

    if (last_button_state == HIGH && button_state == LOW) { // check if button state has changed from high to low
      Serial.println("The button is pressed");  

      led_state = !led_state; // toggle state of LED

      digitalWrite(ledPin, led_state); // control LED arccoding to the toggled state

      delay(100);
    }     

    Serial.println("LED 1:");
    Serial.println(digitalRead(ledPin_owner));
    Serial.println("LED 2:");
    Serial.println(digitalRead(ledPin));

   if (digitalRead(ledPin) == HIGH && digitalRead(ledPin_owner) == HIGH) { // if both LEDs on -> both users have donated
      Serial.println("reset was set to true");
      reset = true;
      displayStart();
    }
    
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println("Card selected");

  // for decoding
  byte blockNumber = 2;
  byte readbackblock[18];

  // Authenticate the block using Key A
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNumber, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // Read the block
  byte bufferSize = sizeof(readbackblock);
  status = mfrc522.MIFARE_Read(blockNumber, readbackblock, &bufferSize);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Read failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // Print the block content
  Serial.print("Block content: ");
  for (int i = 0; i < bufferSize; i++) {
    Serial.write(readbackblock[i]);
  }
  Serial.println();

  // Check if the donation string is present in the block
  bool hasDonated = checkDonation(readbackblock, bufferSize);
  if (hasDonated) {
    Serial.println("User has donated");
    digitalWrite(ledPin_owner, HIGH); // Turn on the LED
  } else {
    Serial.println("User has not donated");
    digitalWrite(ledPin_owner, LOW); // Turn off the LED
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  
}

