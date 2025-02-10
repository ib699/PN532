    /*
  PN532-NFC-RFID-Module-Library
  modified on 18 Nov 2020
  by Amir Mohammad Shojaee @ Electropeak
  Home<iframe class="wp-embedded-content" sandbox="allow-scripts" security="restricted" style="position: absolute; clip: rect(1px, 1px, 1px, 1px);" title="&#8220;Home&#8221; &#8212; Electropeak" src="https://electropeak.com/learn/embed/#?secret=E283jA6nMn" data-secret="E283jA6nMn" width="600" height="338" frameborder="0" marginwidth="0" marginheight="0" scrolling="no"></iframe>

  based on www.electroschematics.com Arduino Examples
*/

#include <SoftwareSerial.h>

#include <PN532_SWHSU.h>

#include <PN532.h>

SoftwareSerial SWSerial( 10, 11 ); // RX, TX

PN532_SWHSU pn532swhsu( SWSerial );

PN532 nfc( pn532swhsu );

// Default Mifare Classic key (6 bytes of 0xFF)
uint8_t keyUniversal[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void setup(void) {
  Serial.begin(115200);
  Serial.println("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // Halt
  }

  // Got valid data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Set the max number of retry attempts to read from a card
  nfc.setPassiveActivationRetries(0xFF);

  // Configure the board to read RFID tags
  nfc.SAMConfig();
}

void loop(void) {
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  // Wait for an ISO14443A type cards (Mifare, etc.)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    Serial.println("Found a card!");
    Serial.print("UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i = 0; i < uidLength; i++) {
      Serial.print(" 0x"); Serial.print(uid[i], HEX);
    }
    Serial.println("");

    // Now try to read all the data from the card
    if (uidLength == 4) {
      // Mifare Classic card
      readMifareClassic(uid, uidLength);
    } else {
      Serial.println("Unsupported card type");
    }

    // Wait a bit before trying again
    delay(1000);
  } else {
    // PN532 probably timed out waiting for a card
    // Serial.println("Timed out waiting for a card");
  }
}

void readMifareClassic(uint8_t *uid, uint8_t uidLength) {
  // Mifare Classic cards have 16 sectors, each with 4 blocks
  for (uint8_t sector = 0; sector < 16; sector++) {
    // Authenticate the sector
    boolean authenticated = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, sector * 4, 1, keyUniversal);
    if (!authenticated) {
      Serial.print("Authentication failed for sector "); Serial.println(sector, DEC);
      continue; // Skip this sector if authentication fails
    }

    // Read all 4 blocks in the sector
    for (uint8_t block = 0; block < 4; block++) {
      uint8_t data[16];
      boolean success = nfc.mifareclassic_ReadDataBlock((sector * 4) + block, data);

      if (success) {
        Serial.print("Sector "); Serial.print(sector, DEC);
        Serial.print(" Block "); Serial.print(block, DEC);
        Serial.print(" Data: ");
        for (uint8_t i = 0; i < 16; i++) {
          Serial.print(" 0x"); Serial.print(data[i], HEX);
        }
        Serial.println("");
      } else {
        Serial.print("Error reading sector "); Serial.print(sector, DEC);
        Serial.print(" block "); Serial.print(block, DEC);
        Serial.println("");
      }
    }
  }
}