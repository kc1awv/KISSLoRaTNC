/*
* KISSLoRaTNC
* See LICENSE file at the top of the source tree.
*
* This product includes software developed by KC1AWV
* Steve Miller, KC1AWV (https://kc1awv.net/).
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. 
* 
* If not, see <https://www.gnu.org/licenses/gpl-3.0.en.html>.
*/

#include <SPI.h>
#include "LoRa.h"
#include "Config.h"
#include "KISS.h"
#include "EEPROM.h"

// modified from https://docs.arduino.cc/learn/programming/eeprom-guide#eeprom-crc
unsigned long settingsCrc(void) {
  const unsigned long crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };

  unsigned long crc = ~0L;

  for (int index = 0 ; index < (int)sizeof(loraSettings)  ; ++index) {
    crc = crc_table[(crc ^ EEPROM[settingsAddress + index]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (EEPROM[settingsAddress + index] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  return crc;
}

void saveSettings() {
  EEPROM.put(settingsAddress, loraSettings);
  EEPROM.put(crcAddress, settingsCrc());
}

void restoreSettings() {
  unsigned long savedCrc;
  EEPROM.get(crcAddress, savedCrc);
  if (settingsCrc() != savedCrc) {
    // bad stored settings, set defaults
    loraSettings.bandwidth = defaultBandwidth;
    loraSettings.codingRate = defaultCodingRate;
    loraSettings.frequency = defaultFrequency;
    loraSettings.spreadingFactor = defaultSpreadingFactor;
    loraSettings.txPower = defaultTxPower;
    saveSettings();
  }
  else {
    EEPROM.get(settingsAddress, loraSettings);
  }
}

void transmit(size_t size) {
  size_t written = 0;

  if (size > MTU) {
    size = MTU;
  }

  LoRa.beginPacket();
  for (size_t i; i < size; i++) {
    LoRa.write(txBuffer[i]);
    written++;
  }
  LoRa.endPacket();

  LoRa.receive();
}

void kissIndicateError(uint8_t errorCode) {
  Serial.write(FEND);
  Serial.write(CMD_ERROR);
  Serial.write(errorCode);
  Serial.write(FEND);
}

void serialCallback(uint8_t txByte) {
  bool setHardware = false;

  if (inFrame && txByte == FEND) {
    if (command == CMD_DATA) {
      inFrame = false;
      //Serial.println("FULL_KISS");
      if (outboundReady) {
          kissIndicateError(ERROR_QUEUE_FULL);
      }
      else {
          outboundReady = true;
          //Serial.println("RDY_OUT");
      }
    }
    else if (command == CMD_HARDWARE) {
      inFrame = false;
      //Serial.println("SET_HARDWARE");
      //Serial.println(frameLength);
      // All commands require at least the command
      if (frameLength >= 1) {
        switch(txBuffer[0]) {
          case HW_SAVE:
            if (frameLength == 1) {
              saveSettings();
              setHardware = true;
            }
          case HW_RESTORE:
            if (frameLength == 1) {
              restoreSettings();
              setHardware = true;
            }
          case HW_SF:
            if (frameLength == 2) {
              loraSettings.spreadingFactor = (int)(txBuffer[1]);
              LoRa.setSpreadingFactor(loraSettings.spreadingFactor);
              setHardware = true;
            }
            break;
          case HW_CR:
            if (frameLength == 2) {
              loraSettings.codingRate = (int)(txBuffer[1]);
              LoRa.setCodingRate4(loraSettings.codingRate);
              setHardware = true;
            }
            break;
          case HW_BW:
            if (frameLength == 5) {
              loraSettings.bandwidth = (uint32_t)(txBuffer[1]) << 24 |
                (uint32_t)(txBuffer[2]) << 16 |
                (uint32_t)(txBuffer[3]) << 8 |
                (uint32_t)(txBuffer[4]);
              LoRa.setSignalBandwidth(loraSettings.bandwidth);
              setHardware = true;
            }
            break;
          case HW_POWER:
            if (frameLength == 2) {
              loraSettings.txPower = (int)(txBuffer[1]);
              LoRa.setTxPower(loraSettings.txPower);
              setHardware = true;
            }
            break;
          case HW_FREQ:
            if (frameLength == 5) {
              loraSettings.frequency = (uint32_t)(txBuffer[1]) << 24 |
                (uint32_t)(txBuffer[2]) << 16 |
                (uint32_t)(txBuffer[3]) << 8 |
                (uint32_t)(txBuffer[4]);
              LoRa.setFrequency(loraSettings.frequency);
              setHardware = true;
            }
            break;
          default:
            break;
        }
      }
      // Signal an error if necessary      
      if (!setHardware) {
        kissIndicateError(ERROR_SETHW);
      }
    }
  }
  else if (txByte == FEND) {
    //Serial.println("KISS_FLAG");
    inFrame = true;
    command = CMD_UNKNOWN;
    frameLength = 0;  
  }
  else if (inFrame && frameLength < MTU) {
    // Get command byte
    if (frameLength == 0 && command == CMD_UNKNOWN) {
      //Serial.println("ACQ_CMD");
      command = txByte;
    }
    else if ((command == CMD_DATA) || (command == CMD_HARDWARE)) {
      if (txByte == FESC) {
        escape = true;
      }
      else { 
        if (escape) {
          if (txByte == TFEND) {
            txByte = FEND;
          }
          if (txByte == TFESC) {
            txByte = FESC;
          }
          escape = false;
        }
        else {
          txBuffer[frameLength++] = txByte;
        }
      }
    }
  }
}

void updateModemStatus() {
  uint8_t status = LoRa.modemStatus();
  lastStatusUpdate = millis();
  if ((status & SIG_DETECT) != 0) {
    statSignalDetected = true;
    //Serial.println("SIG_DETECT");
  }
  else {
    statSignalDetected = false;
  }

  if (statSignalDetected) {
    if (dcdCount < dcdThreshold) {
      dcdCount++;
      dcd = true;
    }
    else {
      dcd = true;
      dcdLed = true;
    }
  }
  else {
    if (dcdCount > 0) {
      dcdCount--;
    }
    else {
      dcdLed = false;
    }
    dcd = false;
  }
}

bool isOutboundReady() {
  return outboundReady;
}

void checkModemStatus() {
  if (millis() - lastStatusUpdate >= statusIntervalms) {
    updateModemStatus();
  }
}

void getPacketData(int packetLength) {
  while (packetLength--) {
    rxBuffer[readLength++] = LoRa.read();
  }
}

void receiveCallback(int packetSize) {
  readLength = 0;
  lastRssi = LoRa.packetRssi();
  getPacketData(packetSize);

  // Send RSSI
  Serial.write(FEND);
  Serial.write(HW_RSSI);
  Serial.write((uint8_t)(lastRssi-rssiOffset));
  Serial.write(FEND);

  // And then write the entire packet
  Serial.write(FEND);
  Serial.write((uint8_t)CMD_DATA);
  for (uint8_t i = 0; i < readLength; i++) {
    uint8_t temp = rxBuffer[i];
    if (temp == FEND) {
      Serial.write(FESC);
      temp = TFEND;
    }
    if (temp == FESC) {
      Serial.write(FESC);
      temp = TFESC;
    }
    Serial.write(temp);
  }
  Serial.write(FEND);
  readLength = 0;
}

void escapedSerialWrite (uint8_t bufferByte) {
  switch(bufferByte) {
    case FEND:
      Serial.write(FESC);
      Serial.write(TFEND);
      break;
    case FESC:
      Serial.write(FESC);
      Serial.write(TFESC);
      break;
    default:
      Serial.write(bufferByte);
  }
}

bool startRadio() {
  if (!LoRa.begin(loraSettings.frequency)) {
    kissIndicateError(ERROR_INITRADIO);
    Serial.println("FAIL");
    while(1);
  }
  else {
    Serial.println("SUCCESS");
    LoRa.setSpreadingFactor(loraSettings.spreadingFactor);
    LoRa.setCodingRate4(loraSettings.codingRate);
    LoRa.setSignalBandwidth(loraSettings.bandwidth);
    LoRa.setTxPower(loraSettings.txPower);
    LoRa.enableCrc();
    LoRa.onReceive(receiveCallback);
    LoRa.receive();
  }
  return 0;
}

void setup() {
  // put your setup code here, to run once:

  // (for testing)
  //for (int i = 0 ; i < (int)EEPROM.length() ; i++) {
  //  EEPROM.write(i, 0);
  //}
  //while(true);

  Serial.begin(serialBaudRate);
  while (!Serial); // Waiting until LoRa32u4 is ready

  // Buffers
  memset(rxBuffer, 0, sizeof(rxBuffer));
  memset(txBuffer, 0, sizeof(txBuffer));

  LoRa.setPins(pinNSS, pinNRST, pinDIO0);

  // Read settings from EEPROM
  restoreSettings();

  startRadio();
}

void loop() {
  // put your main code here, to run repeatedly:
  checkModemStatus();
  if (isOutboundReady() && !SERIAL_READING) {
    if (!dcdWaiting) {
      updateModemStatus();
    }
    if (!dcd && !dcdLed) {
      if (dcdWaiting)
        delay(loraRxTurnaround);
      updateModemStatus();
      if (!dcd) {
        dcdWaiting = false;
        outboundReady = false;
        //Serial.println("CLR_TRANSMIT");
        transmit(frameLength);
      }
      else {
        dcdWaiting = true;
      }
    }
  }
  if (Serial.available()) {
    SERIAL_READING = true;
    char txByte = Serial.read();
    serialCallback(txByte);
    lastSerialRead = millis();
  }

  else {
    if (SERIAL_READING && millis() - lastSerialRead >= serialReadTimeout) {
      SERIAL_READING = false;
    }
  }
}