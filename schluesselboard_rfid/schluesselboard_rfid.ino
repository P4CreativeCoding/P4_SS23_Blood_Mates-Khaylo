#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 26
#define RST_PIN 27
const int LedPin = 12;

MFRC522 mfrc522(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key; 

// // Init array that will store new NUID 
// byte nuidPICC[4];

void setup() { 
  Serial.begin(115200);
  SPI.begin(); // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scans the MIFARE Classic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
  pinMode(LedPin, OUTPUT);
}

int block=2;//this is the block number we will write into and then read. Do not write into 'sector trailer' block, since this can make the block unusable.
byte readbackblock[18];//This array is used for reading out a block. The MIFARE_Read method requires a buffer that is at least 18 bytes to hold the 16 bytes of a block.

void loop() {

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!mfrc522.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been read
  if (!mfrc522.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

    readBlock(block, readbackblock);//read the block back
    Serial.print("read block: ");
    for (int j=0 ; j<16 ; j++)//print the block contents
    {
      Serial.write (readbackblock[j]);//Serial.write() transmits the ASCII numbers as human readable characters to serial monitor
    }
    Serial.println("");
    digitalWrite(LedPin, HIGH);
    delay(1000);
    digitalWrite(LedPin, LOW);
}
//   // Check if the PICC is of Classic MIFARE type
//   if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
//       piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
//       piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
//     Serial.println(F("Your tag is not of type MIFARE Classic."));
//     return;
//   }

//   if (mfrc522.uid.uidByte[0] != nuidPICC[0] ||
//       mfrc522.uid.uidByte[1] != nuidPICC[1] ||
//       mfrc522.uid.uidByte[2] != nuidPICC[2] ||
//       mfrc522.uid.uidByte[3] != nuidPICC[3]) {
//     Serial.println(F("A new card has been detected."));

//     // Store NUID into nuidPICC array
//     for (byte i = 0; i < 4; i++) {
//       nuidPICC[i] = mfrc522.uid.uidByte[i];
//     }

//     Serial.println(F("The NUID tag is:"));
//     Serial.print(F("In hex: "));
//     printHex(mfrc522.uid.uidByte, mfrc522.uid.size);
//     Serial.println();
//     Serial.print(F("In dec: "));
//     printDec(mfrc522.uid.uidByte, mfrc522.uid.size);
//     Serial.println();
//   } else {
//     for (byte i = 0; i < MFRC522::MF_KEY_SIZE; i++) {
//     Serial.print(key.keyByte[i] < 0x10 ? "0" : "");
//     Serial.print(key.keyByte[i], HEX);
//     Serial.print(" ");
//   }
//     Serial.println();
//     Serial.println(F("Card read previously."));
//   }

//   // Halt PICC
//   mfrc522.PICC_HaltA();

//   // Stop encryption on PCD
//   mfrc522.PCD_StopCrypto1();

//   // Check if "has donated" is in Block 2 and control the LED pin accordingly
//   checkBlock2();
// }

// int readBlock(int blockNumber, byte arrayAddress[]) {
//   MFRC522::StatusCode status;
//   byte byteCount;
//   byte buffer[18];

//   byteCount = sizeof(buffer);
//   status = mfrc522.MIFARE_Read(blockNumber, buffer, &byteCount);
//   if (status != MFRC522::STATUS_OK) {
//     Serial.print(F("MIFARE_Read() failed: "));
//     Serial.println(mfrc522.GetStatusCodeName(status));
//     return -1;
//   }

//   for (byte i = 0; i < 16; i++) {
//     arrayAddress[i] = buffer[i];
//   }

//   return 0;
// }

// void checkBlock2() {
//   byte buffer[18];
//   byte block = 2; // Block address of Block 2

//   byte readbackblock[18];
//   readBlock(block, readbackblock); // Read the block back
//   Serial.print("Read block: ");
//   for (int j = 0; j < 16; j++) {
//     Serial.write(readbackblock[j]);
//   }

//   // Authenticate with the block
//   MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &0xFF, &(mfrc522.uid));
//   if (status != MFRC522::STATUS_OK) {
//     Serial.println(F("Authentication failed for Block 2"));
//     return;
//   }

//   // Read data from Block 2
//   byte byteCount = sizeof(buffer);
//   status = mfrc522.MIFARE_Read(block, buffer, &byteCount);
//   if (status != MFRC522::STATUS_OK) {
//     Serial.println(F("Reading failed for Block 2"));
//     return;
//   }

//   // Check if "has donated" is in the block
//   const char expectedText[] = "has donated     ";
//   if (memcmp(buffer, expectedText, sizeof(expectedText) - 1) == 0) {
//     Serial.println(F("Block 2 contains 'has donated'"));
//     digitalWrite(LedPin, HIGH); // Turn on the LED pin
//   } else {
//     Serial.println(F("Block 2 does not contain 'has donated'"));
//     digitalWrite(LedPin, LOW); // Turn off the LED pin
//   }
// }

void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

int readBlock(int blockNumber, byte arrayAddress[]) 
{
  int largestModulo4Number=blockNumber/4*4;
  int trailerBlock=largestModulo4Number+3;//determine trailer block for the sector

  /*****************************************authentication of the desired block for access***********************************************************/
  byte status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  //byte PCD_Authenticate(byte command, byte blockAddr, MIFARE_Key *key, Uid *uid);
  //this method is used to authenticate a certain block for writing or reading
  //command: See enumerations above -> PICC_CMD_MF_AUTH_KEY_A	= 0x60 (=1100000),		// this command performs authentication with Key A
  //blockAddr is the number of the block from 0 to 15.
  //MIFARE_Key *key is a pointer to the MIFARE_Key struct defined above, this struct needs to be defined for each block. New cards have all A/B= FF FF FF FF FF FF
  //Uid *uid is a pointer to the UID struct that contains the user ID of the card.
  if (status != MFRC522::STATUS_OK) {
         Serial.print("PCD_Authenticate() failed (read): ");
         Serial.println(mfrc522.GetStatusCodeName(status));
         return 3;//return "3" as error message
  }
  //it appears the authentication needs to be made before every block read/write within a specific sector.
  //If a different sector is being authenticated access to the previous one is lost.


  /*****************************************reading a block***********************************************************/
        
  byte buffersize = 18;//we need to define a variable with the read buffer size, since the MIFARE_Read method below needs a pointer to the variable that contains the size... 
  status = mfrc522.MIFARE_Read(blockNumber, arrayAddress, &buffersize);//&buffersize is a pointer to the buffersize variable; MIFARE_Read requires a pointer instead of just a number
  if (status != MFRC522::STATUS_OK) {
          Serial.print("MIFARE_read() failed: ");
          Serial.println(mfrc522.GetStatusCodeName(status));
          return 4;//return "4" as error message
  }
  Serial.println("block was read");
}

// void printDec(byte *buffer, byte bufferSize) {
//   for (byte i = 0; i < bufferSize; i++) {
//     Serial.print(buffer[i] < 0x10 ? " 0" : " ");
//     Serial.print(buffer[i], DEC);
//   }
}
