# ParasiticV1p0 firmware - PIC18F16Q41
#
#   make            build build/parasitic.hex
#   make flash      program with a PICkit3 (scripting firmware) via pk2cmd
#   make clean
#
# Override tool locations on the command line or in the environment, e.g.
#   make XC8=/opt/microchip/xc8/v4.00/bin/xc8-cc DFP=/path/to/PIC18F-Q_DFP/1.31.492

XC8     ?= $(HOME)/microchip/xc8/v4.00/bin/xc8-cc
DFP     ?= $(HOME)/microchip/packs/Microchip/PIC18F-Q_DFP/1.31.492
PK2DIR  ?= $(HOME)/microchip/pk2cmd
CHIP    := 18F16Q41

SRC     := $(wildcard src/*.c) lib/fatfs/ff.c
OUT     := build
HEX     := $(OUT)/parasitic.hex

CFLAGS  := -mcpu=$(CHIP) -mdfp=$(DFP)/xc8 -Os -std=c99 -Wall \
           -msummary=-psect,-class,+mem,-hex,-file

all: $(HEX)

$(HEX): $(SRC) $(wildcard src/*.h) $(wildcard lib/fatfs/*.h) | $(OUT)
	$(XC8) $(CFLAGS) -o $@ $(SRC)

$(OUT):
	mkdir -p $@

# -PPIC18F16Q41 : part   -F : hex file   -M : program all   -Y : verify
# -W            : the board supplies its own power (do not power from PICkit)
# -R            : release MCLR afterwards so the firmware runs
flash: $(HEX)
	cd $(PK2DIR) && ./pk2cmd -PPIC18F16Q41 -W -R -F$(abspath $(HEX)) -M -Y

clean:
	rm -rf $(OUT)

.PHONY: all flash clean
