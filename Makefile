ifeq "$(CONFIG)" ""
	CONFIG=Release
endif

# Build directory
BUILD_DIR = $(CONFIG)

# Compiler
CXX = g++

# Compilation flags (O2 - to compile faster)
CXXFLAGS = -std=c++17 -O2

ifeq "$(CONFIG)" "Debug"
	CXXFLAGS += -g -Wall
endif

# Execution file target
TARGET = main

# List of source files
SOURCES = *.cpp vulkan/*.cpp vulkan/types/*.cpp

# Vulkan support
VULKAN = -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

dirs:
	-mkdir -p $(BUILD_DIR)

all: dirs
	$(CXX) $(CXXFLAGS) $(SOURCES) -lSDL2 ${VULKAN} -o $(BUILD_DIR)/$(TARGET)

clean:
	rm -f $(TARGET)