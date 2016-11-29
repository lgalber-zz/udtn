all:
	${MAKE} TARGET=avr-zigbit NOAVRSIZE=1 -f Makefile.zigbit collector.elf
	avr-objcopy -O ihex -R .signature -R .fuse -R .eeprom collector.elf collector.hex	
	avr-objcopy -j .eeprom --no-change-warnings --change-section-lma .eeprom=0 -O ihex collector.elf  "collector.eep"  
	avr-size -C --mcu=atmega1281 collector.elf

clean:
	${MAKE} -f Makefile.zigbit clean
	rm -rf obj_avr-zigbit
	rm symbols.c symbols.h collector.elf collector.hex
