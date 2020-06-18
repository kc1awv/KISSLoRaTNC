# KISSLoRaTNC

[![GPLv3 License](https://img.shields.io/badge/License-GPL%20v3-yellow.svg)](https://opensource.org/licenses/)
[![Build Status](https://travis-ci.org/kc1awv/KISSLoRaTNC.svg?branch=master)](https://travis-ci.org/kc1awv/KISSLoRaTNC)

[![GitHub issues](https://img.shields.io/github/issues/kc1awv/KISSLoRaTNC)](https://github.com/kc1awv/KISSLoRaTNC/issues)
[![GitHub forks](https://img.shields.io/github/forks/kc1awv/KISSLoRaTNC)](https://github.com/kc1awv/KISSLoRaTNC/network)
[![GitHub stars](https://img.shields.io/github/stars/kc1awv/KISSLoRaTNC)](https://github.com/kc1awv/KISSLoRaTNC/stargazers)
![GitHub last commit](https://img.shields.io/github/last-commit/kc1awv/KISSLoRaTNC)


## Arduino based LoRa KISS TNC

Currently, build is failing for MoteinoMEGA boards due to an out-of-date pin definition in platformio. See [issue #199 in platform/atmelavr](https://github.com/platformio/platform-atmelavr/issues/199)

To fix locally, go to ~/.platformio/packages/framework-arduino-avr/variants/moteinomega/pins_arduino.h

Add in this line at line #72:

    #define digitalPinToInterrupt(p) ((p) == 10? 0: (p) == 11? 1: (p) == 2? 2: NOT_AN_INTERRUPT)
    
Uncomment the last block in platformio.ini and then build again.

---

KISSLoRaTNC is a work derived from code written by [Sandeep Mistry](https://github.com/sandeepmistry/arduino-LoRa) and [Mark Qvist](https://github.com/markqvist)

This code was developed using [VSCode](https://code.visualstudio.com/) and [Platformio](https://platformio.org/).

KISSLoRaTNC is developed for the:
- Arduino Uno, using the [HamShield: LoRa Edition](https://inductivetwig.com/products/hamshield-lora-edition-high-power)
- Arduino Micro, using the [Adafruit RFM96W LoRa Radio Breakout](https://www.adafruit.com/product/3073)
- [MoteinoMEGA-USB with the RFM96 Transceiver](https://lowpowerlab.com/shop/product/138)

The Config.h file contains the pinouts for the different Arduino-based boards. These pinouts are specific to the microcontroller board and LoRa device pairings in the list above. If you decide to use a different pairing, please adjust Config.h accordingly. Also, please remember that the power output should also be adjusted, as the RFM96W has a max Tx Power value of 20dBm. The HamShield: LoRa Edition 440MHz can be set to 30dBm (1 watt).

"Default" settings are:
- Frequency: 434.450 MHz
- Spreading Factor: 8
- Coding Rate: 7
- Signal Bandwidth: 125 kHz
- Tx Power: 20 (can be set to 30 for HamShield)
- Serial: 38400, 8N1

Basically, after checking the Config.h file for correctness, build and upload to your Arduino-based board.

Useful applications:
- [Xastir](https://xastir.org/index.php/Main_Page) (APRS)
- [tncattach](https://github.com/markqvist/tncattach) (Ethernet compatible kissattach replacement)
- [BPQ32](http://www.cantab.net/users/john.wiseman/Documents/index.html) (Amateur Radio BBS Application)