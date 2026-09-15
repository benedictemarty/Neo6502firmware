# Image multi-boot du Neo6502 (F-80) : sélecteur + firmware Neo (slot 0) + reload BBC (slot 1) + reload Oric (slot 2).
#   make -C multiboot image        -> multiboot/neo6502-multi.uf2 (à glisser sur RPI-RP2, BOOTSEL enfoncé)
#   make -C multiboot check        -> vérifie sur libemul (Phosphoneo test-multiboot)
# Les images reload viennent du fork (~/reload-emulator, branche bbc), configuré avec
#   cmake -S platforms/rp2040 -B platforms/rp2040/build_mb -DNEO_MULTIBOOT_DIR=$(CURDIR) -DNEO_SLOT_BBC=1 -DNEO_SLOT_ORIC=2
SDK      ?= $(CURDIR)/../firmware/build/_deps/pico_sdk-src
RELOAD   ?= $(HOME)/reload-emulator/platforms/rp2040
RL_BUILD  = $(RELOAD)/build_mb
BBC_UF2   = $(RL_BUILD)/systems/bbc/bbc.uf2
ORIC_UF2  = $(RL_BUILD)/systems/oric/oric.uf2
SLOTS     = --slot 0 ../firmware/firmware_slot0.uf2 $(if $(wildcard $(BBC_UF2)),--slot 1 $(BBC_UF2)) $(if $(wildcard $(ORIC_UF2)),--slot 2 $(ORIC_UF2))

image: build/neoboot.uf2 ../firmware/firmware_slot0.uf2 reload
	python3 mkimage.py -o neo6502-multi.uf2 --selector build/neoboot.uf2 $(SLOTS)

build/neoboot.uf2: neoboot.c neoboot.h memmap_selector.ld CMakeLists.txt
	cmake -S . -B build -DPICO_SDK_PATH=$(SDK) >/dev/null
	$(MAKE) -C build neoboot >/dev/null

../firmware/firmware_slot0.uf2:
	$(MAKE) -C ../firmware build-multiboot STORAGE=USB

reload:
	@if [ -d "$(RELOAD)" ]; then \
		[ -d "$(RL_BUILD)" ] || cmake -S $(RELOAD) -B $(RL_BUILD) -DNEO_MULTIBOOT_DIR=$(CURDIR) -DNEO_SLOT_BBC=1 -DNEO_SLOT_ORIC=2 >/dev/null; \
		$(MAKE) -C $(RL_BUILD) bbc oric >/dev/null 2>&1 || echo "reload : compilation bbc/oric échouée (slots 1/2 omis)"; \
	else echo "reload-emulator absent : slots 1/2 omis"; fi

check:
	$(MAKE) -C $(HOME)/Phosphoneo test-multiboot

clean:
	rm -rf build neo6502-multi.uf2

.PHONY: image reload check clean
