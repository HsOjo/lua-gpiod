# Makefile for complete gpiod Lua binding

# Use Lua 5.4
LUA_VERSION := 5.4
LUA_INCDIR := /usr/include/lua$(LUA_VERSION)
LUA_LIBDIR := /usr/lib/aarch64-linux-gnu

# Compiler settings
CC = gcc
CFLAGS = -O2 -fPIC -Wall -Wextra
LDFLAGS = -shared

# Include paths and libraries
INCLUDES = -I$(LUA_INCDIR)
LIBS = -lgpiod

# Target files
TARGET = gpiod.so
SOURCE = gpiod_lua.c

# Default target
all: $(TARGET)

# Build rule
$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $< $(LIBS)

# Clean
clean:
	rm -f $(TARGET)

# Install (optional)
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/lib/lua/$(LUA_VERSION)/

# Test
test: $(TARGET)
	lua$(LUA_VERSION) -e "local gpiod = require('gpiod'); print('gpiod version:', gpiod.version()); print('Module loaded successfully')"

.PHONY: all clean install test