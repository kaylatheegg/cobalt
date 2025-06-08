SRCPATHS = \
	src/*.c \
	src/common/*.c \
	src/parse/*.c

SRC = $(wildcard $(SRCPATHS))
OBJECTS = $(SRC:src/%.c=build/%.o)

EXECUTABLE_NAME = cobalt
ECHO = echo

ifeq ($(OS),Windows_NT)
	EXECUTABLE_NAME = $(EXECUTABLE_NAME).exe
endif

CC = gcc
LD = gcc

INCLUDEPATHS = -Isrc -Isrc/common
DEBUGFLAGS = -pg -g
ASANFLAGS = -fsanitize=undefined -fsanitize=address
CFLAGS = -std=c2x -MD -D_XOPEN_SOURCE=700 -fwrapv \
		 -fno-delete-null-pointer-checks -fno-strict-overflow -fno-strict-aliasing \
		 -Wall -Wno-format -Wno-unused -Werror=incompatible-pointer-types -Wno-discarded-qualifiers \

OPT = -O3

FILE_NUM = 0

build/%.o: src/%.c
	$(eval FILE_NUM=$(shell echo $$(($(FILE_NUM)+1))))
	$(shell $(ECHO) 1>&2 -e "\e[0m[\e[32m$(FILE_NUM)/$(words $(SRC))\e[0m]\t Compiling \e[1m$<\e[0m")
	@$(CC) -c -o $@ $< $(INCLUDEPATHS) $(CFLAGS) $(OPT)

build: $(OBJECTS)
	@echo Linking with $(LD)...
	@$(LD) $(OBJECTS) -o $(EXECUTABLE_NAME) $(CFLAGS) -lm
	@echo Successfully built: $(EXECUTABLE_NAME)

buildlibc:
	make -C stdlib clean
	make -C stdlib

debug: CFLAGS += $(DEBUGFLAGS)
debug: OPT = -O0
debug: build

clean:
	@rm -rf build/
	@mkdir build/
	@mkdir -p $(dir $(OBJECTS))

cleanbuild: clean build

test: build
	cd tests; ./run.sh


-include $(OBJECTS:.o=.d)