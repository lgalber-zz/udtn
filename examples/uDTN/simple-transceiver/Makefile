all:
	${MAKE} TARGET=avr-zigbit NOAVRSIZE=1 -f Makefile.zigbit simple-transceiver.elf
	avr-objcopy -O ihex -R .signature -R .fuse -R .eeprom simple-transceiver.elf simple-transceiver.hex	
	avr-objcopy -j .eeprom --no-change-warnings --change-section-lma .eeprom=0 -O ihex simple-transceiver.elf  "simple-transceiver.eep"  
	avr-size -C --mcu=atmega1281 simple-transceiver.elf

clean:
	${MAKE} -f Makefile.zigbit clean
	rm -rf obj_avr-zigbit
	rm symbols.c symbols.h simple-transceiver.elf simple-transceiver.hex
