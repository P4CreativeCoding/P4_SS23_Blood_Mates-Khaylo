#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 21
#define RST_PIN 22
const int LedPin = 12;

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key; 

// Init array that will store new NUID 
byte nuidPICC[4];

void setup() { 
  Serial.begin(115200);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scans the MIFARE Classic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
  pinMode(LedPin, OUTPUT);
}
 
void loop() {

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been read
  if (!rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check if the PICC is of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] ||
      rfid.uid.uidByte[1] != nuidPICC[1] ||
      rfid.uid.uidByte[2] != nuidPICC[2] ||
      rfid.uid.uidByte[3] != nuidPICC[3]) {
    Serial.println(F("A new card has been detected."));

    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }

    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
    Serial.print(F("In dec: "));
    printDec(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
  } else {
    Serial.println(F("Card read previously."));
  }

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();

  // Check if "has donated" is in Block 2 and control the LED pin accordingly
  checkBlock2();
}

int readBlock(int blockNumber, byte arrayAddress[]) {
  MFRC522::StatusCode status;
  byte byteCount;
  byte buffer[18];

  byteCount = sizeof(buffer);
  status = rfid.MIFARE_Read(blockNumber, buffer, &byteCount);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(rfid.GetStatusCodeName(status));
    return -1;
  }

  for (byte i = 0; i < 16; i++) {
    arrayAddress[i] = buffer[i];
  }

  return 0;
}

void checkBlock2() {
  byte buffer[18];
  byte block = 2; // Block address of Block 2

  byte readbackblock[18];
  readBlock(block, readbackblock); // Read the block back
  Serial.print("Read block: ");
  for (int j = 0; j < 16; j++) {
    Serial.write(readbackblock[j]);
  }

  // Authenticate with the block
  MFRC522::StatusCode status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.println(F("Authentication failed for Block 2"));
    return;
  }

  // Read data from Block 2
  byte byteCount = sizeof(buffer);
  status = rfid.MIFARE_Read(block, buffer, &byteCount);
  if (status != MFRC522::STATUS_OK) {
    Serial.println(F("Reading failed for Block 2"));
    return;
  }

  // Check if "has donated" is in the block
  const char expectedText[] = "has donated     ";
  if (memcmp(buffer, expectedText, sizeof(expectedText) - 1) == 0) {
    Serial.println(F("Block 2 contains 'has donated'"));
    digitalWrite(LedPin, HIGH); // Turn on the LED pin
  } else {
    Serial.println(F("Block 2 does not contain 'has donated'"));
    digitalWrite(LedPin, LOW); // Turn off the LED pin
  }
}

void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}
