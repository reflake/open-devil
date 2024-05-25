ifeq "$(CONFIG)" ""
	CONFIG=Release
endif

# Build directory
BUILD_DIR = $(CONFIG)

# Compiler
CXX = g++

# Compilation flags 
CXXFLAGS = -std=c++17

ifeq "$(CONFIG)" "Debug"
	CXXFLAGS += -g -Wall -O0
else
# O2 - to compile faster
	CXXFLAGS += -O2
endif

# Linker flags
LDXXFLAGS = -lm

# Execution file target
TARGET = main
TARGET_BUILD_PATH = $(BUILD_DIR)/$(TARGET)

# Sources
SOURCE_DIR = src

# Objects
OBJECTS = \
	$(BUILD_OBJ_DIR)/main.o \
	$(BUILD_OBJ_DIR)/file.o \
	$(BUILD_OBJ_DIR)/application.o \
	$(BUILD_OBJ_DIR)/vulkan/shader.o \
	$(BUILD_OBJ_DIR)/vulkan/engine.o \
	$(BUILD_OBJ_DIR)/vulkan/types/qfamily_indices.o \
	$(BUILD_OBJ_DIR)/vulkan/types/swap_chain_support.o \
	$(BUILD_OBJ_DIR)/vulkan/types/image_params.o \
	$(BUILD_OBJ_DIR)/vulkan/types/vertex.o \
	$(BUILD_OBJ_DIR)/media/image.o \
	$(BUILD_OBJ_DIR)/media/model.o \

OBJECT_DIRS = \
	$(BUILD_OBJ_DIR)/vulkan \
	$(BUILD_OBJ_DIR)/vulkan/types \
	$(BUILD_OBJ_DIR)/media \

BUILD_OBJ_DIR = $(BUILD_DIR)/obj

# Vulkan support
VULKAN = -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

# SDL support
SDL = -lSDL2

COMP = $(CXX) $(CXXFLAGS) -o $@ -c $<

# Shaders
SHADERS = \
	$(BUILD_SHADER_DIR)/main.frag.spv \
	$(BUILD_SHADER_DIR)/main.vert.spv \

BUILD_SHADER_DIR = $(BUILD_DIR)/shaders

SHADER_SRC_DIR = shaders

# Texture
TEXTURE = $(BUILD_TEX_DIR)/sample.png

BUILD_TEX_DIR = $(BUILD_DIR)/textures

TEXTURE_DIR = textures

# Png Library
LIBPNG = -lpng

# Assimp asset import library
ASSIMP = -lassimp

all: dirs $(TARGET_BUILD_PATH)

$(TARGET_BUILD_PATH): $(OBJECTS) $(SHADERS) $(TEXTURE)
	$(CXX) $(OBJECTS) $(LDXXFLAGS) $(SDL) $(VULKAN) $(LIBPNG) $(ASSIMP) -o $(TARGET_BUILD_PATH)

dirs:
	-mkdir -p $(BUILD_DIR)
	-mkdir -p $(BUILD_OBJ_DIR)
	-mkdir -p $(OBJECT_DIRS)
	-mkdir -p $(BUILD_SHADER_DIR)
	-mkdir -p $(BUILD_TEX_DIR)
	$(MV_SHADERS)

clean:
	rm -rf $(BUILD_DIR)

$(BUILD_OBJ_DIR)/%.o : $(SOURCE_DIR)/%.cpp
	$(COMP)

$(BUILD_SHADER_DIR)/%.spv : $(SHADER_SRC_DIR)/%
	glslc $< -o $@

$(BUILD_TEX_DIR)/% : $(TEXTURE_DIR)/%
	cp $< $@
