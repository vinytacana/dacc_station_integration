CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -I. -Iinclude -MMD -MP
AR := ar
ARFLAGS := rcs

LIB_TARGET := libdacc-config.a
BUILD_DIR := build
TEST_BT_BIN := tests/bin/test_bluetooth_parser
TEST_NETWORK_BIN := tests/bin/test_network_parser
TEST_DISPLAY_BIN := tests/bin/test_display_parser
TEST_BINS := $(TEST_BT_BIN) $(TEST_NETWORK_BIN) $(TEST_DISPLAY_BIN)

SRCS_LIB := SystemControl.cpp AudioControl.cpp DisplayControl.cpp NetworkControl.cpp ConfigCommand.cpp BluetoothInternal.cpp BluetoothControl.cpp Capabilities.cpp
OBJS_LIB := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRCS_LIB))
DEPS_LIB := $(OBJS_LIB:.o=.d)

.PHONY: all clean test

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

test: $(TEST_BINS)
	./$(TEST_BT_BIN)
	./$(TEST_NETWORK_BIN)
	./$(TEST_DISPLAY_BIN)

clean:
	rm -rf $(BUILD_DIR) $(LIB_TARGET) $(TEST_BINS)
	rm -f *.o *.d

-include $(DEPS_LIB)
