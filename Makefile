GBDK_HOME ?= /opt/gbdk
LCC ?= $(GBDK_HOME)/bin/lcc
TEST_CC ?= cc
PYBOY_PYTHON ?= python3

TARGET := build/gb-win31.gbc
INCLUDES := -Iinclude
ROM_FLAGS := -Wm-yC -Wm-yn"GB WORKBENCH" -Wl-j
CFLAGS := $(ROM_FLAGS) $(INCLUDES)

ROM_SRCS := \
	src/main.c \
	src/input.c \
	src/assets.c \
	src/ui.c \
	src/desktop_text.c \
	src/boot.c \
	src/desktop.c \
	src/audio.c \
	src/minesweeper_model.c \
	src/minesweeper.c \
	src/paint.c \
	src/piano.c \
	src/media.c \
	src/cannon.c

.PHONY: all clean test verify screens visual-test smoke-test

all: $(TARGET)

$(TARGET): $(ROM_SRCS) $(wildcard include/*.h) | build
	$(LCC) $(CFLAGS) -o $@ $(ROM_SRCS)

build:
	mkdir -p $@

build/test-minesweeper: src/minesweeper_model.c tests/test_minesweeper_model.c include/minesweeper_model.h | build
	$(TEST_CC) -std=c99 -Wall -Wextra -Werror $(INCLUDES) -o $@ src/minesweeper_model.c tests/test_minesweeper_model.c

test: build/test-minesweeper
	./build/test-minesweeper

verify: $(TARGET)
	python3 tools/verify_rom.py $(TARGET)

screens: $(TARGET)
	$(PYBOY_PYTHON) tools/capture_screens.py $(TARGET) --out build/screens

visual-test: screens
	$(PYBOY_PYTHON) tools/compare_screens.py build/screens tests/golden

smoke-test: $(TARGET)
	$(PYBOY_PYTHON) tools/smoke_interactions.py $(TARGET)

clean:
	$(RM) -r build
