PROJECT  = main
PORT     = /dev/serial/by-id/usb-Silicon_Labs_myAVR_-_mySmartUSB_light_mySmartUSBlight-0001-if00-port0
MCU      = attiny45
PROTOCOL = stk500v2
DEPS     = mydefs.h myserial.h
CFLAGS   = -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Wall -Wextra


.PHONY: all, compile, asm, clean, flash, on, off, 3, 5, reset

########################################################
# compile  program

#build, transmit, put power on
all: off $(PROJECT).hex flash on


compile: $(PROJECT).elf

$(PROJECT).elf: $(PROJECT).c $(DEPS)
	@echo "compile...."
	@avr-gcc -mmcu=$(MCU) -Os $(CFLAGS) -g $(PROJECT).c -o $(PROJECT).elf


$(PROJECT).hex: $(PROJECT).elf
	@echo "objcopy...."
	@avr-objcopy -O ihex -R .eeprom $(PROJECT).elf $(PROJECT).hex


asm:
	@echo "compile to asm...."
	@avr-gcc -mmcu=$(MCU)  $(CFLAGS) -O0 $(PROJECT).c -S -o $(PROJECT).asm

$(PROJECT).asm: $(PROJECT).c $(DEPS)
	@echo "compile to asm...."
	@avr-gcc -mmcu=$(MCU)  $(CFLAGS) -O0 $(PROJECT).c -S -o $(PROJECT).asm


clean:
	rm -f $(PROJECT).hex $(PROJECT).asm $(PROJECT).elf

########################################################
# transmit program

flash: $(PROJECT).hex
	@echo "transmit...."
	@avrdude -c $(PROTOCOL) -p $(MCU) -P $(PORT) -U flash:w:$(PROJECT).hex:i

########################################################
# extra functions of mySmartUSB light.
# see http://shop.myavr.com/index.php?ws=download_file.ws.php&dlid=216&filename=software/tool_mySmartUSB-Kommandos_de_en_fr.zip

PREFIX = "\xE6\xB5\xBA\xB9\xB2\xB3\xA9"

on:
	@echo "power on...."
	@env echo -e ${PREFIX}+ > $(PORT)
off:
	@echo "power off...."
	@env echo -e ${PREFIX}- > $(PORT)
3:
	@echo "set voltage to 3V...."
	@env echo -e ${PREFIX}3 > $(PORT)
5:
	@echo "set voltage to 5V...."
	@env echo -e ${PREFIX}5 > $(PORT)
rM:
	@echo "reset MCU"
	@env echo -e ${PREFIX}r > $(PORT)
rP:
	@echo "reset Programmer"
	@env echo -e ${PREFIX}R > $(PORT)

.PHONY : on off
