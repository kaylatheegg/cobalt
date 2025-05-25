SRCPATHS = \
	src/ctype/*.c 

SRC = $(wildcard $(SRCPATHS))
OBJECTS = $(SRC:src/%.c=build/%.o)

EXECUTABLE_NAME = slibc.so
ECHO = echo

ifeq ($(OS),Windows_NT)
	EXECUTABLE_NAME = $(EXECUTABLE_NAME).exe
else
	ECHO = /usr/bin/echo
	# JANK FIX FOR SANDWICH'S DUMB ECHO ON HIS LINUX MACHINE
endif

CC = gcc
LD = gcc

INCLUDEPATHS = -Iinclude
DEBUGFLAGS = -pg -g
ASANFLAGS = -fsanitize=undefined -fsanitize=address
CFLAGS = -std=c17 -MD -D_XOPEN_SOURCE=700 -fwrapv \
		 -fno-delete-null-pointer-checks -fno-strict-overflow -fno-strict-aliasing \
		 -Wall -Wno-format -Wno-unused -Werror=incompatible-pointer-types -Wno-discarded-qualifiers \

OPT = -O3

FILE_NUM = 0

build/%.o: src/%.c
	$(eval FILE_NUM=$(shell echo $$(($(FILE_NUM)+1))))
	$(shell $(ECHO) 1>&2 -e "\e[0m[\e[32m$(FILE_NUM)/$(words $(SRC))\e[0m]\t Compiling \e[1m$<\e[0m")
	@$(CC) -c -o $@ $< $(INCLUDEPATHS) $(CFLAGS) -fPIC $(OPT)

build: $(OBJECTS)
	@echo Linking with $(LD)...
	@$(LD) $(OBJECTS) -shared -o $(EXECUTABLE_NAME) $(CFLAGS) -lm
	@echo Successfully built: $(EXECUTABLE_NAME)

debug: CFLAGS += $(DEBUGFLAGS)
debug: OPT = -O0
debug: build

clean:
	@rm -rf build/
	@mkdir build/
	@mkdir -p $(dir $(OBJECTS))

cleanbuild: clean build

-include $(OBJECTS:.o=.d)