# Cobalt config

BUILD_DIR = build

EXECUTABLE_NAME = cobalt
ECHO = echo

SRCPATHS = \
	src/*.c \
	src/parse/*.c

CC ?= gcc
LD = $(CC)

# libcommon config
export COMMON_OUT_DIR=../$(BUILD_DIR)

SRC = $(wildcard $(SRCPATHS))
OBJECTS = $(SRC:src/%.c=build/%.o)

ifeq ($(OS),Windows_NT)
	EXECUTABLE_NAME = $(EXECUTABLE_NAME).exe
endif

INCLUDEPATHS = -Isrc -Icommon/include
DEBUGFLAGS = -pg -g
ASANFLAGS = -fsanitize=undefined -fsanitize=address
CFLAGS = -std=c2x -MD -D_XOPEN_SOURCE=700 -fwrapv \
		 -fno-delete-null-pointer-checks -fno-strict-overflow -fno-strict-aliasing \
		 -Wall -Wno-format -Wno-unused -Werror=incompatible-pointer-types -Wno-discarded-qualifiers \

OPT = -O3

FILE_NUM = 0

.PHONY: all
all: common buildlibc cobalt

.PHONY: common
common: bin/libcommon.a
bin/libcommon.a:
	$(MAKE) -C common
	cp $(BUILD_DIR)/libcommon.a bin/libcommon.a

build/%.o: src/%.c
	$(eval FILE_NUM=$(shell echo $$(($(FILE_NUM)+1))))
	$(shell $(ECHO) 1>&2 -e "\e[0m[\e[32m$(FILE_NUM)/$(words $(SRC))\e[0m]\t Compiling \e[1m$<\e[0m")
	@$(CC) -c -o $@ $< $(INCLUDEPATHS) $(CFLAGS) $(OPT)

.PHONY: cobalt
cobalt: build/$(EXECUTABLE_NAME)
build/$(EXECUTABLE_NAME): bin/libcommon.a $(OBJECTS)
	@echo Linking with $(LD)...
	@$(LD) $(OBJECTS) -o bin/$(EXECUTABLE_NAME) $(CFLAGS) -lm -Lbin -lcommon
	@echo Successfully built: bin/$(EXECUTABLE_NAME)

buildlibc:
	make -C stdlib clean
	make -C stdlib

debug: CFLAGS += $(DEBUGFLAGS)
debug: OPT = -O0
debug: build

fuzz: CC = clang
fuzz: LD = clang
fuzz: OPT = -O0
fuzz: CFLAGS += -fsanitize=fuzzer -DFUZZ -g
fuzz: build

clean:
	$(MAKE) -C common clean
	@rm -rf build/
	@rm -rf bin/
	@mkdir build/
	@mkdir -p $(dir $(OBJECTS))
	@mkdir bin

test: build
	cd tests; ./run.sh

-include $(OBJECTS:.o=.d)