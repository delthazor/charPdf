CXX = g++
# Third-party headers under include/ (PDFWriter, FreeType, etc.): treat as system so
# libc++ deprecation noise (e.g. char_traits<unsigned char> in ByteList.h) is suppressed
# until upstream fixes it. Project code stays under -I./src and is not affected.
CXXFLAGS = -std=c++20 -Wall -Wextra -MMD -MP -I./src -isystem ./include
LDFLAGS = -L./lib
LIBS = -lPDFWriter -lFreeType -lLibJpeg -lLibPng -lLibTiff -lZlib -lLibAesgm -lz -lm

SRC_DIR = src
ASSETS_DIR = assets
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
TARGET = $(BUILD_DIR)/pdf_app

SOURCES = $(shell find $(SRC_DIR) -name '*.cpp')
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
DEPS = $(OBJECTS:.o=.d)

.PHONY: all clean run

all: $(TARGET)

# Runtime loads paths like assets/... relative to cwd; make run uses $(BUILD_DIR).
$(BUILD_DIR)/assets: | $(BUILD_DIR)
	@if [ -e "$(BUILD_DIR)/assets" ] && [ ! -L "$(BUILD_DIR)/assets" ]; then \
		echo "error: $(BUILD_DIR)/assets exists and is not a symlink; remove or merge with $(ASSETS_DIR)" >&2; exit 1; \
	fi
	ln -snf ../$(ASSETS_DIR) "$(BUILD_DIR)/assets"

$(TARGET): $(OBJECTS) | $(BUILD_DIR) $(BUILD_DIR)/assets
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

run: $(TARGET)
	@cd $(BUILD_DIR) && ./$(notdir $(TARGET))

-include $(DEPS)

clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(BUILD_DIR)/*.pdf

info:
	@echo "Compiler: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo "Libraries: $(LIBS)"
	@echo "Sources: $(SOURCES)"
	@echo "Objects: $(OBJECTS)"
	@echo "Target: $(TARGET)"

