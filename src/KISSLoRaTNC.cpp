#include <SPI.h>
#include "LoRa.h"
#include "Config.h"
#include "KISS.h"

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
  if (inFrame && txByte == FEND && command == CMD_DATA) {
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
    else if (command == CMD_DATA) {
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
  if (status & (SIG_DETECT == 0x01)) {
    statSignalDetected = true;
  }
  else {
    statSignalDetected = false;
  }
  if (status & (SIG_SYNCED == 0x01)) {
    statSignalSynced = true;
  }
  else {
    statSignalSynced = false;
  }
  if (status & (RX_ONGOING == 0x01)) {
    statRxOngoing = true;
  }
  else {
    statRxOngoing = false;
  }

  if (statSignalDetected || statSignalSynced || statRxOngoing) {
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
  if (!LoRa.begin(loraFrequency)) {
    kissIndicateError(ERROR_INITRADIO);
    Serial.println("FAIL");
    while(1);
  }
  else {
    Serial.println("SUCCESS");
    LoRa.setSpreadingFactor(loraSpreadingFactor);
    LoRa.setCodingRate4(loraCodingRate);
    LoRa.enableCrc();
    LoRa.onReceive(receiveCallback);
    LoRa.receive();
  }
  return 0;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(serialBaudRate);
  while (!Serial); // Waiting until LoRa32u4 is ready

  // Buffers
  memset(rxBuffer, 0, sizeof(rxBuffer));
  memset(txBuffer, 0, sizeof(txBuffer));
  
  LoRa.setPins(pinNSS, pinNRST, pinDIO0);
    
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