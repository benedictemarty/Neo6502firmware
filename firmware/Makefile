# ***************************************************************************************
# ***************************************************************************************
#
#		Name : 		Makefile
#		Author :	Paul Robson (paul@robsons.org.uk)
#		Date : 		20th November 2023
#		Reviewed :	No
#		Purpose :	Main firmware makefile, most of the work is done by CMake.
#
# ***************************************************************************************
# ***************************************************************************************

ifeq ($(OS),Windows_NT)
#is this really needed?  WSL2 is the 2023.
include ..\build_env\common.make
else
include ../build_env/common.make
endif

BINARY = firmware
#
#		Can be SDCARD or USB currently. If you change this use make switch to
#		rebuild as it cmake clean doesn't clean things up enough (probably me)
#
STORAGE = USB

all: build

always:

# ***************************************************************************************
#
#									Rebuild the kernel		
#
# ***************************************************************************************

kernel: always
	$(MAKE) -C ..$(S)kernel
	$(CCOPY) $(BINDIR)kernel_binary.h $(COMDIR)include$(S)data
	$(HCONVERT) $(NEODOSBIN) $(COMDIR)include$(S)data$(S)neodos_binary.h neodos B800
	$(PYTHON) $(COMDIR)scripts$(S)latin1.py $(COMDIR)include$(S)interface$(S)font_5x7.h >$(COMDIR)include$(S)data$(S)latin1font.h
	$(TOUCH) sources$(S)memory.c*

# ***************************************************************************************
#
#									Rebuild the firmware
#
# ***************************************************************************************

switch: 
	rm -Rf build
	$(MAKE) rebuild 

rebuild: clean build

# ***************************************************************************************
#
#							  Remake the preliminary files
#
# ***************************************************************************************

prelim: kernel
	$(MAKE) -B -C $(COMDIR)
	
# ***************************************************************************************
#
#									Build the firmware
#
# ***************************************************************************************

build: prelim always
	cmake --fresh -B build -DSTORAGE_TYPE=$(STORAGE)
	$(MAKE) -C build
	$(CCOPY) build$(S)sources$(S)$(BINARY).elf .
	$(CCOPY) build$(S)sources$(S)$(BINARY).uf2 .
	$(MAKE) size

# ***************************************************************************************
#
#				Memory budget (T-13) : RAM = .data + .bss on the 256 KB of main SRAM.
#				The build fails above RAM_LIMIT bytes (see README-TRINITY.md, docs/BACKLOG.md).
#
# ***************************************************************************************

#				2026-09-24 (T-58) : 230000 -> 231000. La conversion de palette par mots de 32 bits
#				ajoute ~120 o de code en RAM, pour diviser par deux les transactions mémoire du
#				callback de ligne — ce qui est exactement la ressource qui manque à core 1.
#				Marge vérifiée : il reste alors 31 Ko de tas, dont ~11,5 Ko de tampons TMDS
#				(malloc) ; la pile de core 0 vit dans le scratch, hors .bss, et celle de core 1
#				(2 Ko) y est déjà comptée. Le seuil reste bien en deçà des 262 144 o de SRAM.
#				2026-09-24 (T-73) : 231000 -> 231600. Le port de debug UART coûte 876 o : un anneau
#				de 512 o en .bss, et DBGWrite/DBGFlush/DBGTelemetry en RAM — obligatoire, puisque
#				DBGFlush est appelée depuis DSPSync et que rien de ce qu'appelle DSPSync ne doit
#				vivre en flash (T-46). Sans ce relèvement il restait 80 o de marge, trop peu pour
#				travailler. Marge vérifiée : 31,2 Ko de tas restants, tampons TMDS compris.
#
#				2026-09-25 (T-74) : 231600 -> 233500. Le terminal porte l'anneau de 512 o à 2 Ko :
#				la console produit ~1,5 Ko d'un coup sur un DIR, et le port ne sort que ~3 Ko/s
#				(32 o de FIFO à 95 Hz), donc un anneau plus petit perdrait justement la trace des
#				moments chargés — ceux qu'on veut observer. Marge vérifiée : 29,2 Ko de tas
#				restants, dont ~11,5 Ko de tampons TMDS ; la pile de core 0 est dans le scratch.
#
#				2026-09-25 (T-74) : 233500 -> 234500. Les rapports !v !k !p ajoutent ~340 o de code
#				en RAM — obligatoire là encore, puisqu'ils partent de DBGPoll, donc de DSPSync.
#				Ils ne laissaient que 228 o de marge, trop peu pour la suite. Marge vérifiée :
#				28,9 Ko de tas restants.
RAM_LIMIT ?= 234500

size: always
	arm-none-eabi-size -A $(BINARY).elf | grep -E "^\.(text|rodata|data|bss) "
	@ram=$$(arm-none-eabi-size -A $(BINARY).elf | awk '$$1==".data"||$$1==".bss"||$$1==".ram_vector_table" {s+=$$2} END {print s}') ; \
	echo "RAM (.data+.bss+vecteurs) : $$ram octets sur 262144, limite $(RAM_LIMIT), libres (tas) : $$((262144 - ram))" ; \
	if [ $$ram -gt $(RAM_LIMIT) ] ; then echo "*** RAM_LIMIT depasse (T-13)" ; exit 1 ; fi

# ***************************************************************************************
#
#								Clean build directories
#
# ***************************************************************************************

clean: always
	cmake --fresh -B build -DSTORAGE_TYPE=$(STORAGE)
	cmake --build build --target clean

# ***************************************************************************************
#
#								Build & Upload firmware	
#
# ***************************************************************************************

run: build upload

upload: 
	$(UPLOADER) $(UPCONFIG) $(UPCOMMANDS)

# ***************************************************************************************
#
#				Send last BASIC program translated over the serial port.
#
# ***************************************************************************************

checkserial:
	$(PYTHON) $(BINDIR)nxmit.zip /dev/ttyUSB0 b:$(BASICDIR)basic$(S)build$(S)tokenised.dat

# ***************************************************************************************
#
#									Reset RP2040
#
# ***************************************************************************************

reset:
	$(UPLOADER) $(UPCONFIG) -c "adapter speed 5000" -c "init" -c "reset init" -c "reset run" -c "exit"

# ***************************************************************************************
#
#								Build both firmware
#
# ***************************************************************************************

dual: prelim always pythonapps
	$(CDEL)   firmware.elf firmware.uf2

	cmake -B build -DSTORAGE_TYPE=SDCARD
	cmake --build build --target clean
	$(MAKE) -C build
	$(CCOPY) build$(S)sources$(S)$(BINARY).elf $(BINDIR)$(BINARY)_sd.elf
	$(CCOPY) build$(S)sources$(S)$(BINARY).uf2 $(BINDIR)$(BINARY)_sd.uf2

	cmake -B build -DSTORAGE_TYPE=USB
	cmake --build build --target clean
	$(MAKE) -C build
	$(CCOPY) build$(S)sources$(S)$(BINARY).elf $(BINDIR)$(BINARY)_usb.elf
	$(CCOPY) build$(S)sources$(S)$(BINARY).uf2 $(BINDIR)$(BINARY)_usb.uf2

pythonapps:
	$(CMAKEDIR) build
	$(CCOPY) scripts$(S)nxmit.py build$(S)__main__.py
	zip -j $(BINDIR)nxmit.zip build$(S)__main__.py
	$(CDEL) build$(S)*.p
		
# ***************************************************************************************
#
#									Release rebuild
#
# ***************************************************************************************

release:
	$(MAKE) -B dual
