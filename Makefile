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
EDITOR_TARGET = $(BUILD_DIR)/char_editor

SOURCES = $(shell find $(SRC_DIR) -name '*.cpp')
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
DEPS = $(OBJECTS:.o=.d)

.PHONY: all clean run editor run_editor check-character-json sync-webrender-cfg serve-webrender webrender

PARAM ?=

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
	@cd $(BUILD_DIR) && ./$(notdir $(TARGET)) $(PARAM)

check-character-json: $(TARGET) | $(BUILD_DIR)/assets
	@cd $(BUILD_DIR) && ./$(notdir $(TARGET)) $(PARAM)

# Character JSON editor (Qt6 Widgets, optional)
QT_CFLAGS := $(shell pkg-config --cflags Qt6Widgets 2>/dev/null)
QT_LIBS := $(shell pkg-config --libs Qt6Widgets 2>/dev/null)
EDITOR_SRC_DIR = tools/char_editor/src
EDITOR_SOURCES = $(shell find $(EDITOR_SRC_DIR) -name '*.cpp')
EDITOR_SHARED_SOURCES = src/syshelpers/AcCalculation.cpp src/syshelpers/Utilities.cpp
EDITOR_OBJECTS = $(patsubst $(EDITOR_SRC_DIR)/%.cpp,$(OBJ_DIR)/char_editor/%.o,$(EDITOR_SOURCES)) \
	$(OBJ_DIR)/char_editor/shared/AcCalculation.o $(OBJ_DIR)/char_editor/shared/Utilities.o
EDITOR_DEPS = $(EDITOR_OBJECTS:.o=.d)

.PHONY: editor
editor: $(EDITOR_TARGET)

$(EDITOR_TARGET): $(EDITOR_OBJECTS) | $(BUILD_DIR)
	@if [ -z "$(QT_CFLAGS)" ] || [ -z "$(QT_LIBS)" ]; then \
		echo "error: Qt6Widgets not found via pkg-config. Install Qt6 + pkg-config and ensure Qt6Widgets.pc is visible." >&2; \
		exit 1; \
	fi
	$(CXX) $(EDITOR_OBJECTS) $(LDFLAGS) $(QT_LIBS) -o $@

$(OBJ_DIR)/char_editor/%.o: $(EDITOR_SRC_DIR)/%.cpp | $(OBJ_DIR)
	@if [ -z "$(QT_CFLAGS)" ]; then \
		echo "error: Qt6Widgets not found via pkg-config. Install Qt6 + pkg-config and ensure Qt6Widgets.pc is visible." >&2; \
		exit 1; \
	fi
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(QT_CFLAGS) -I./$(EDITOR_SRC_DIR) -c $< -o $@

$(OBJ_DIR)/char_editor/shared/AcCalculation.o: src/syshelpers/AcCalculation.cpp | $(OBJ_DIR)
	@if [ -z "$(QT_CFLAGS)" ]; then \
		echo "error: Qt6Widgets not found via pkg-config. Install Qt6 + pkg-config and ensure Qt6Widgets.pc is visible." >&2; \
		exit 1; \
	fi
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(QT_CFLAGS) -c $< -o $@

$(OBJ_DIR)/char_editor/shared/Utilities.o: src/syshelpers/Utilities.cpp | $(OBJ_DIR)
	@if [ -z "$(QT_CFLAGS)" ]; then \
		echo "error: Qt6Widgets not found via pkg-config. Install Qt6 + pkg-config and ensure Qt6Widgets.pc is visible." >&2; \
		exit 1; \
	fi
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(QT_CFLAGS) -c $< -o $@

run_editor: $(EDITOR_TARGET) | $(BUILD_DIR)/assets
	@cd $(BUILD_DIR) && ./$(notdir $(EDITOR_TARGET))

-include $(DEPS)
-include $(EDITOR_DEPS)

clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(EDITOR_TARGET) $(BUILD_DIR)/chars $(BUILD_DIR)/*.pdf

WEBRENDER_DIR = tools/webrender
SYNC_WEBRENDER = $(WEBRENDER_DIR)/scripts/sync-webrender.sh

sync-webrender-cfg:
	@$(SYNC_WEBRENDER)

serve-webrender: sync-webrender-cfg
	@cd $(WEBRENDER_DIR) && python3 -m http.server 8765

webrender: serve-webrender

info:
	@echo "Compiler: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo "Libraries: $(LIBS)"
	@echo "Sources: $(SOURCES)"
	@echo "Objects: $(OBJECTS)"
	@echo "Target: $(TARGET)"

