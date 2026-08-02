CXX ?= g++
CC ?= gcc
HL2SDK ?= ../hl2dm-src

TARGET := build/hl2mp_shutdown_fix.so
SOURCES := src/hl2mp_shutdown_fix.cpp
OBJECTS := $(SOURCES:src/%.cpp=build/%.o)

CPPFLAGS := -Isrc -isystem $(HL2SDK)/public -isystem $(HL2SDK)/public/tier0 \
	-D_GNU_SOURCE -D_LINUX -DLINUX -DPOSIX -DGNUC -DCOMPILER_GCC -DHL2MP -DPLATFORM_64BITS -DX64BITS
CXXFLAGS := -std=c++17 -m64 -O2 -g -fPIC -fvisibility=hidden -fno-exceptions -fno-rtti -Wall -Wextra -Werror -Wno-unknown-pragmas
LDFLAGS := -shared -m64 -Wl,-z,defs -Wl,-z,relro,-z,now -Wl,--version-script=src/exports.map
LIBS := -ldl

all: $(TARGET)

build:
	mkdir -p build

build/%.o: src/%.cpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

clean:
	rm -rf build

.PHONY: all clean
