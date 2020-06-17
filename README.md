[![Build Status](https://travis-ci.org/kc1awv/KISSLoRaTNC.svg?branch=master)](https://travis-ci.org/kc1awv/KISSLoRaTNC)

# KISSLoRaTNC
Arduino based LoRa KISS TNC

Currently, build is failing for MoteinoMEGA boards due to an out-of-date pin definition in platformio. See issue #199 in platform/atmelavr https://github.com/platformio/platform-atmelavr/issues/199

To fix locally, go to ~/.platformio/packages/framework-arduino-avr/variants/moteinomega/pins_arduino.h

Add in this line at line #72:

    #define digitalPinToInterrupt(p) ((p) == 10? 0: (p) == 11? 1: (p) == 2? 2: NOT_AN_INTERRUPT)
    
Then build again.
