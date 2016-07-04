PORT=/dev/ttyUSB1

OBJECTS=rfduino-makefile/RFduino/libraries/RFduinoBLE/RFduinoBLE.o \
	rfduino-makefile/RFduino/libraries/Servo/Servo.o \
	lcd/LiquidCrystal.o main.o \
	aes/AES.o \
	crypto/crypto.o \
	config/config.o \
	process/process.o \
	lcd/LcdPrinter.o

HEADERS=-Irfduino-makefile/RFduino/libraries/RFduinoBLE -Irfduino-makefile/RFduino/libraries/Servo -I.
#CXX_FLAGS=-D __PRINT_LOG__

#export CXX_FLAGS
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
build:
	$(MAKE) -C rfduino-makefile rfduino_lib build
