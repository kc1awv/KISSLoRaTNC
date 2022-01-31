#include <stdint.h>
#include <stdlib.h>

#ifndef KISS_H
  #define KISS_H

  #define FEND              0xC0
  #define FESC              0xDB
  #define TFEND             0xDC
  #define TFESC             0xDD

  #define CMD_UNKNOWN       0xFE
  #define CMD_DATA          0x00
  #define CMD_HARDWARE      0x06

  #define HW_RSSI           0x21
  #define HW_SF             0x22  // LoRa Spreading Factor - one byte value
  #define HW_CR             0x23  // LoRa Coding Rate - one byte value
  #define HW_BW             0x24  // LoRa Bandwidth - four byte value (MSB first)
  #define HW_POWER          0x25  // LoRa Transmit Power - one byte value
  #define HW_FREQ           0x26  // LoRa Frequency - four byte value (MSB first)
  #define HW_SAVE           0x27  // Save hardware parameters in EEPROM
  #define HW_RESTORE        0x28  // Restore hardware parameters from EEPROM

  #define CMD_ERROR         0x90
  #define ERROR_INITRADIO   0x01
  #define ERROR_TXFAILED    0x02
  #define ERROR_QUEUE_FULL  0x04
  #define ERROR_SETHW       0x05

  size_t frameLength;
  bool inFrame                = false;
  bool escape                 = false;
  bool SERIAL_READING         = false;
  uint8_t command             = CMD_UNKNOWN;
  uint32_t lastSerialRead     = 0;
  uint32_t serialReadTimeout  = 25;

#endif