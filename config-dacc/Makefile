CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -I. -Iinclude -MMD -MP
AR := ar
ARFLAGS := rcs

LIB_TARGET := libdacc-config.a
BUILD_DIR := build
TEST_BT_BIN := tests/bin/test_bluetooth_parser
TEST_NETWORK_BIN := tests/bin/test_network_parser
TEST_DISPLAY_BIN := tests/bin/test_display_parser
TEST_DISPLAY_CONTROL_BIN := tests/bin/test_display_control
TEST_AUDIO_BIN := tests/bin/test_audio_parser
TEST_CONFIG_COMMAND_BIN := tests/bin/test_config_command
CAPABILITIES_SMOKE_BIN := tests/bin/capabilities_smoke
TEST_BINS := $(TEST_BT_BIN) $(TEST_NETWORK_BIN) $(TEST_DISPLAY_BIN) $(TEST_DISPLAY_CONTROL_BIN) $(TEST_AUDIO_BIN) $(TEST_CONFIG_COMMAND_BIN)

SRCS_LIB := SystemControl.cpp AudioControl.cpp DisplayControl.cpp NetworkControl.cpp ConfigCommand.cpp BluetoothInternal.cpp BluetoothControl.cpp Capabilities.cpp
OBJS_LIB := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRCS_LIB))
DEPS_LIB := $(OBJS_LIB:.o=.d)

.PHONY: all clean test capabilities-smoke

all: $(LIB_TARGET)

$(LIB_TARGET): $(OBJS_LIB)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_BT_BIN): tests/test_bluetooth_parser.cpp BluetoothInternal.cpp include/config-dacc/functions.hpp BluetoothInternal.hpp
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) tests/test_bluetooth_parser.cpp BluetoothInternal.cpp -o $@

$(TEST_NETWORK_BIN): tests/test_network_parser.cpp include/config-dacc/NetworkParsing.hpp
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) tests/test_network_parser.cpp -o $@

$(TEST_DISPLAY_BIN): tests/test_display_parser.cpp include/config-dacc/DisplayParsing.hpp include/config-dacc/functions.hpp
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) tests/test_display_parser.cpp -o $@

$(TEST_DISPLAY_CONTROL_BIN): tests/test_display_control.cpp DisplayControl.cpp ConfigCommand.cpp include/config-dacc/functions.hpp
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) tests/test_display_control.cpp DisplayControl.cpp ConfigCommand.cpp -o $@

$(TEST_AUDIO_BIN): tests/test_audio_parser.cpp include/config-dacc/AudioParsing.hpp include/config-dacc/functions.hpp
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) tests/test_audio_parser.cpp -o $@

$(TEST_CONFIG_COMMAND_BIN): tests/test_config_command.cpp ConfigCommand.cpp include/config-dacc/functions.hpp
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) tests/test_config_command.cpp ConfigCommand.cpp -o $@

$(CAPABILITIES_SMOKE_BIN): tests/capabilities_smoke.cpp $(LIB_TARGET) include/config-dacc/functions.hpp
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) tests/capabilities_smoke.cpp $(LIB_TARGET) -o $@

test: $(TEST_BINS)
	./$(TEST_BT_BIN)
	./$(TEST_NETWORK_BIN)
	./$(TEST_DISPLAY_BIN)
	./$(TEST_DISPLAY_CONTROL_BIN)
	./$(TEST_AUDIO_BIN)
	./$(TEST_CONFIG_COMMAND_BIN)

capabilities-smoke: $(CAPABILITIES_SMOKE_BIN)

clean:
	rm -rf $(BUILD_DIR) $(LIB_TARGET) $(TEST_BINS) $(CAPABILITIES_SMOKE_BIN)
	rm -f *.o *.d

-include $(DEPS_LIB)
