CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -I. -MMD -MP
AR := ar
ARFLAGS := rcs


LIB_TARGET := libdacc-config.a
TEST_BIN := tests/bin/test_bluetooth_parser

SRCS_LIB := HardwareControl.cpp ConfigCommand.cpp BluetoothInternal.cpp BluetoothControl.cpp
OBJS_LIB := $(SRCS_LIB:.cpp=.o)
DEPS_LIB := $(OBJS_LIB:.o=.d)

.PHONY: all clean test

all: $(LIB_TARGET)

$(LIB_TARGET): $(OBJS_LIB)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_BIN): tests/test_bluetooth_parser.cpp BluetoothInternal.cpp functions.hpp BluetoothInternal.hpp
	@mkdir -p tests/bin
	$(CXX) $(CXXFLAGS) tests/test_bluetooth_parser.cpp BluetoothInternal.cpp -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f *.o *.d $(LIB_TARGET) $(TEST_BIN)

-include $(DEPS_LIB)
