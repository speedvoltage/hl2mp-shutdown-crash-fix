CXX ?= g++
CC ?= gcc
HL2SDK ?= ../source-sdk-2013/src

TARGET := build/srcds_shutdown_fix.so
SOURCES := src/srcds_shutdown_fix.cpp
OBJECTS := $(SOURCES:src/%.cpp=build/%.o)

CPPFLAGS := -Isrc -isystem $(HL2SDK)/public -isystem $(HL2SDK)/public/tier0 \
	-D_GNU_SOURCE -D_LINUX -DLINUX -DPOSIX -DGNUC -DCOMPILER_GCC -DPLATFORM_64BITS -DX64BITS
CXXFLAGS := -std=c++17 -m64 -O2 -g -fPIC -fvisibility=hidden -fno-exceptions -fno-rtti -Wall -Wextra -Werror -Wno-unknown-pragmas
LDFLAGS := -shared -m64 -Wl,-z,defs -Wl,-z,relro,-z,now -Wl,--version-script=src/exports.map
LIBS := -Wl,--no-as-needed -l:libdl.so.2 -Wl,--as-needed

all: $(TARGET)

build:
	mkdir -p build

build/%.o: src/%.cpp src/metamod_minimal.h | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS) src/exports.map
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

clean:
	rm -rf build

.PHONY: all clean
