# ***************************************************************************************
# ***************************************************************************************
#
# Name     : Makefile
# Author   : Paul Robson (paul@robsons.org.uk)
# Date     : 20th November 2023
# Reviewed : No
# Purpose  : Main firmware makefile, most of the work is done by CMake.
#
# ***************************************************************************************
# ***************************************************************************************

ifeq ($(OS),Windows_NT)
include build_env\common.make
else
include build_env/common.make
endif


# ***************************************************************************************
#
# Remake everything to release state
#
# ***************************************************************************************

all: firmware-deps emulator-deps-nix emulator-deps-win
	$(CMAKEDIR) bin
	@echo building firmware
	$(MAKE) -B -C kernel release
	$(MAKE) -B -C $(BASICDIR) release FWDIR=$(ROOTDIR)
	$(MAKE) -B -C firmware release
	@echo building emulators
	$(MAKE) -B -C emulator release
	$(MAKE) -B -C examples release
	@echo building release package
	$(MAKE) -B -C release


# ***************************************************************************************
#
# Make firmware only
#
# ***************************************************************************************

firmware: firmware-deps
	@echo building firmware
	$(CMAKEDIR) bin
	$(MAKE) -B -C kernel release
	$(MAKE) -B -C firmware release


# ***************************************************************************************
#
# Make emulator only
#
# ***************************************************************************************

windows: emulator-deps-nix emulator-deps-win
	@echo building windows emulator
	$(CMAKEDIR) bin
	$(MAKE) -B -C kernel
	$(MAKE) -B -C emulator clean
	$(MAKE) -B -C emulator ewindows
	$(MAKE) -B -C examples release

linux: emulator-deps-nix
	@echo building nix emulator
	$(CMAKEDIR) bin
	$(MAKE) -B -C kernel
	$(MAKE) -B -C emulator clean
	$(MAKE) -B -C emulator elinux
	$(MAKE) -B -C examples release

macos: emulator-deps-nix
	@echo building macos emulator
	make -B -C emulator emacos
	make -B -C examples release




# ***************************************************************************************
#
# Verify that dependencies are installed
#
# ***************************************************************************************

firmware-deps:
	@echo checking for firmware dependencies:
	@cmake             --version
	@g++               --version
	@arm-none-eabi-g++ --version
	@# NOTE: this is not accounting for 'arm-none-eabi-newlib'

emulator-deps-win:
	@x86_64-w64-mingw32-g++ --version

emulator-deps-nix:
	@echo checking for emulator dependencies:
	@g++         --version
	@64tass      --version
	@sdl2-config --version
	@zip         --version
	@python3     --version
	@python3 -c 'from importlib.metadata import version ; pkg="gitpython" ; print("python-%s: %s" % (pkg , version(pkg)))'
	@python3 -c 'from importlib.metadata import version ; pkg="pillow" ; print("python-%s: %s" % (pkg , version(pkg)))'

# ***************************************************************************************
#
# Clean everything
#
# ***************************************************************************************

# Tests de la toolbox (T-12) : tests/toolbox/*.asm joués dans bin/neo, journal comparé à *.expected
test-toolbox:
	for t in tests/toolbox/*.asm; do tests/toolbox/run_neo.sh $$(basename $$t .asm) || exit 1; done

# Tests de l'API Trinity hors toolbox (tests/api/*.asm : 3,27, 1,20-21...)
test-api:
	for t in tests/api/*.asm; do TESTDIR=tests/api tests/toolbox/run_neo.sh $$(basename $$t .asm) || exit 1; done

# Tests de démarrage dans neo (T-37, ADR-0001 f) : tests/boot/run_boot.sh
test-boot:
	tests/boot/run_boot.sh

# Synthétiseur compilé sur PC (T-80) : sndcreator.cpp contre un common.h minimal
test-snd:
	@mkdir -p build
	g++ -O2 -Wall -Itests/snd -Ifirmware/common/include tests/snd/test_sndcreator.cpp firmware/common/sources/interface/sndcreator.cpp -o build/test_sndcreator
	build/test_sndcreator

# Outils carte (tests/api/outils/*.asm) : assemblés puis emballés en .NEO dans ~/neo-carte/cle-usb.
# Une cible, parce que la 0.10.5 a corrigé late.asm sans pouvoir le reconstruire (routine cr retirée
# par erreur, assemblage cassé) et que la clé a gardé des mois un binaire périmé.
CLEUSB ?= $(HOME)/neo-carte/cle-usb
MKNEO  ?= $(NEODOSDIR)tools/mkneo.py

outils:
	@mkdir -p $(CLEUSB)
	@for t in tests/api/outils/*.asm; do \
		n=$$(basename $$t .asm); \
		64tass --mw65c02 --nostart -q -o $(CLEUSB)/$$n.neo6502 $$t || exit 1; \
		$(PYTHON) $(MKNEO) $(CLEUSB)/$$n.neo6502 $(CLEUSB)/$$(echo $$n | tr a-z A-Z).NEO 800 800 $$n || exit 1; \
	done

# Tests API pour la carte (T-98) : tests/api/*.asm assemblés avec NEO = 0 (retour par RTS à NeoDOS, journal
# laissé en $2000), emballés en .NEO (nom tronqué à 8 lettres), avec leurs attendus et fichiers, dans
# $(CLEUSB)/TESTS. Sans les tests à crochets neo (.args) ou à modem factice (.modem). Joués par
# firmware/scripts/neotests.sh (SWD).
tests-carte:
	@rm -rf $(CLEUSB)/TESTS && mkdir -p $(CLEUSB)/TESTS/src
	@for t in tests/api/*.asm; do \
		n=$$(basename $$t .asm); \
		[ -e tests/api/$$n.args ] || [ -e tests/api/$$n.modem ] || [ -e tests/api/$$n.nocarte ] && continue; \
		N=$$(echo $$n | cut -c1-8 | tr a-z A-Z); \
		64tass --mw65c02 --nostart -q -o $(CLEUSB)/TESTS/src/$$n.neo6502 $$t || exit 1; \
		$(PYTHON) $(MKNEO) $(CLEUSB)/TESTS/src/$$n.neo6502 $(CLEUSB)/TESTS/$$N.NEO 800 800 $$n || exit 1; \
		cp tests/api/$$n.expected* $(CLEUSB)/TESTS/src/; cp tests/api/$$n.carte.expected.re $(CLEUSB)/TESTS/src/ 2>/dev/null; \
		[ -e tests/api/$$n.bin ] && cp tests/api/$$n.bin $(CLEUSB)/TESTS/; \
		echo "$$N $$n" >> $(CLEUSB)/TESTS/src/liste.txt; \
	done; echo "tests-carte : $$(wc -l < $(CLEUSB)/TESTS/src/liste.txt) tests dans $(CLEUSB)/TESTS"

clean:
	$(MAKE) -B -C kernel clean
	$(MAKE) -B -C $(BASICDIR) clean
	$(MAKE) -B -C emulator clean
	$(MAKE) -B -C firmware clean

