GBDK_HOME ?= /opt/gbdk
LCC ?= $(GBDK_HOME)/bin/lcc
ROMUSAGE ?= $(GBDK_HOME)/bin/romusage
TEST_CC ?= cc
PYBOY_PYTHON ?= python3

TARGET := build/gb-win31.gbc
OBJDIR := build/obj
INCLUDES := -Iinclude
CFLAGS := $(INCLUDES)
# MBC5 + 8 KiB battery SRAM, CGB-only, with automatic bank assignment for
# every source that declares `#pragma bank 255`.
ROM_FLAGS := -Wm-yC -Wm-yn"GB WORKBENCH" -Wm-yt0x1B -Wm-ya2 -Wl-j \
	-autobank -Wb-ext=.rel

ROM_SRCS := \
	src/main.c \
	src/input.c \
	src/assets.c \
	src/text.c \
	src/text_font.c \
	src/ui.c \
	src/boot.c \
	src/desktop.c \
	src/audio.c \
	src/minesweeper_model.c \
	src/minesweeper.c \
	src/paint.c \
	src/piano.c \
	src/media.c \
	src/cannon.c
ROM_OBJS := $(ROM_SRCS:src/%.c=$(OBJDIR)/%.o)
HEADERS := $(wildcard include/*.h)

.PHONY: all clean test verify budget screens visual-test smoke-test font font-check golden montage

all: $(TARGET)

$(OBJDIR)/%.o: src/%.c $(HEADERS) | $(OBJDIR)
	$(LCC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(ROM_OBJS) | build
	$(LCC) $(ROM_FLAGS) -o $@ $(ROM_OBJS)

build $(OBJDIR):
	mkdir -p $@

# The generated font table is committed so ROM builds never need Python.
font:
	python3 tools/fontgen.py assets/system_font.txt src/text_font.c

font-check:
	python3 tools/fontgen.py assets/system_font.txt src/text_font.c --check

build/test-minesweeper: src/minesweeper_model.c tests/test_minesweeper_model.c include/minesweeper_model.h | build
	$(TEST_CC) -std=c99 -Wall -Wextra -Werror $(INCLUDES) -o $@ src/minesweeper_model.c tests/test_minesweeper_model.c

test: build/test-minesweeper font-check
	./build/test-minesweeper

verify: $(TARGET)
	python3 tools/verify_rom.py $(TARGET)

budget: $(TARGET)
	$(ROMUSAGE) build/gb-win31.map -g

screens: $(TARGET)
	$(PYBOY_PYTHON) tools/capture_screens.py $(TARGET) --out build/screens

visual-test: screens
	$(PYBOY_PYTHON) tools/compare_screens.py build/screens tests/golden

smoke-test: $(TARGET)
	$(PYBOY_PYTHON) tools/smoke_interactions.py $(TARGET)

# Deliberately accept the current frames as the new visual baseline.
golden: screens
	cp build/screens/*.png tests/golden/

montage: screens
	$(PYBOY_PYTHON) tools/make_montage.py build/screens docs/screens/current-montage.png

clean:
	$(RM) -r build
