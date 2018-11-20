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
