PORT=/dev/ttyUSB0
OBJECTS=rfduino-makefile/RFduino/libraries/RFduinoBLE/RFduinoBLE.o rfduino-makefile/RFduino/libraries/Servo/Servo.o main.o 
HEADERS=-Irfduino-makefile/RFduino/libraries/RFduinoBLE -Irfduino-makefile/RFduino/libraries/Servo -I.

export PORT
export OBJECTS
export HEADERS

.PHONY: all

all:
	$(MAKE) -C rfduino-makefile

clean:
	$(MAKE) -C rfduino-makefile clean

distclean:
	$(MAKE) -C rfduino-makefile distclean