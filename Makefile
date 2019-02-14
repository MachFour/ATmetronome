TARGET = metronome

### BOARD_TAG
### It must be set to the board you are currently using. (i.e uno, mega2560, etc.)
BOARD_TAG = uno
BOARD_SUB = atmega328
VARIANT = standard
NO_CORE = Yes

# When using NO_CORE, have to define these variables
HEX_MAXIMUM_SIZE = 28672
MCU = atmega328p
F_CPU = 8000000L


#uno.bootloader.tool=avrdude
#uno.bootloader.low_fuses=0xFF
#uno.bootloader.high_fuses=0xDE
#uno.bootloader.extended_fuses=0xFD
#uno.bootloader.unlock_bits=0x3F
#uno.bootloader.lock_bits=0x0F
#ISP_EXT_FUSE=0xfd
ISP_LOCK_FUSE_PRE=0x3f
ISP_LOCK_FUSE_POST=0x0f

# From ATMega328p datasheet
# crystal oscillator, BOD enabled:
# 	CKSEL0 = 1, SUT[1:0] = 01
# crystal oscillator, BOD disabled, slow start:
#   CKSEL0 = 1, SUT[1:0] = 11
# low power crystal oscillator, 3-8MHz:
#   CKSEL[3:1] = 110
# low power crystal oscillator, 8-16MHz:
#   CKSEL[3:1] = 111
# full swing crystal oscillator:
#   CKSEL[3:1] = 011
#
# LOW FUSE BYTES:
# CKDIV8 CKOUT SUT1  SUT0  CKSEL3 CKSEL2 CKSEL1 CKSEL0
ISP_LOW_FUSE=0xdf
ISP_HIGH_FUSE=0xde
# change brown out level to ~1.8V: BODLEVEL[2:0]=110 -> EFUSE=0xfe
# 0b11111110
# disable brown out detection: BODLEVEL[2:0]=111 -> EFUSE=0xff
ISP_EXT_FUSE=0xfe
BOOTLOADER_FILE=optiboot/optiboot_atmega328.hex

# optimise dat
OPTIMIZATION_LEVEL=2

# override arduino libraries
ARDUINO_LIBS =

### MONITOR_BAUDRATE
### It must be set to Serial baudrate value you are using.
MONITOR_BAUDRATE  = 115200

PROJECT_DIR       = /home/max/devel/metronome/v3

ISP_PORT = usb
ISP_PROG = usbasp
AVRDUDE_ISP_BAUDRATE = 19200

# When using NO_CORE, have to define these variables
AVRDUDE_ARD_PROGRAMMER  = arduino
AVRDUDE_ARD_BAUDRATE    = 115200
AVRDUDE_BOOTLOADER_FILE = /usr/share/arduino/hardware/archlinux-arduino/avr/bootloaders/optiboot/optiboot_atmega328.hex

include /home/max/devel/arduino/Arduino-Makefile-max.mk

# !!! Important. You have to use make ispload to upload when using ISP programmer
