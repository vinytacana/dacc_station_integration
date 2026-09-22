
# Local: Faculdade/dacc_station/dacc-station-os/ui/Makefile

CXX = g++

# Includes (Onde achar os .hpp)
CXXFLAGS = -std=c++17 -Wall -Wextra -g \
           -Iinclude \
           -I../config-dacc \
           -I../config-dacc/include \
           -I../ipc/include \
           -I../logs/include \
           -I../process-manager/third-party \
           -isystem ../logs/third-party/spdlog/include \
           -MMD -MP

# Bibliotecas (Onde achar os .a e quais linkar)
LDFLAGS = ../config-dacc/libdacc-config.a \
          ../logs/libdacc-logs.a \
          ../ipc/libdacc-ipc.a \
          -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_gfx -lSDL2_mixer -pthread

SRC_DIR = src
OBJ_DIR = build
BIN_DIR = ../bin
BIN = $(BIN_DIR)/dacc-ui
TEST_SYSTEM_STATUS_BIN = tests/bin/test_system_status
TEST_CONFIG_LAYOUT_BIN = tests/bin/test_config_layout
TEST_BINS = $(TEST_SYSTEM_STATUS_BIN) $(TEST_CONFIG_LAYOUT_BIN)

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

all: directories $(BIN)

$(BIN): $(OBJS) ../config-dacc/libdacc-config.a ../logs/libdacc-logs.a ../ipc/libdacc-ipc.a
	@echo "Linkando dacc-ui..."
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compilando UI: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

$(TEST_SYSTEM_STATUS_BIN): tests/test_system_status.cpp src/SystemStatus.cpp include/SystemStatus.hpp ../config-dacc/include/config-dacc/functions.hpp
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) tests/test_system_status.cpp src/SystemStatus.cpp -o $@ -lSDL2 -pthread

$(TEST_CONFIG_LAYOUT_BIN): tests/test_config_layout.cpp src/ConfigLayout.cpp include/ConfigLayout.hpp include/JanelaAudioEVideo.hpp include/LayoutEvents.hpp
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) tests/test_config_layout.cpp src/ConfigLayout.cpp -o $@

test: $(TEST_BINS)
	./$(TEST_SYSTEM_STATUS_BIN)
	./$(TEST_CONFIG_LAYOUT_BIN)

clean:
	rm -rf $(OBJ_DIR) $(BIN) $(TEST_BINS)

.PHONY: all clean directories test

-include $(DEPS)
