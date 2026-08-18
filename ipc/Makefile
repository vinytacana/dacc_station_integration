CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -Iinclude -MMD -MP
AR := ar
ARFLAGS := rcs

BUILD_DIR := build
LIB_TARGET := libdacc-ipc.a
TEST_BIN := tests/bin/test_framed_socket

SRCS := src/FramedSocket.cpp
OBJS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean test

all: $(LIB_TARGET)

$(LIB_TARGET): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_BIN): tests/test_framed_socket.cpp $(LIB_TARGET)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $< $(LIB_TARGET) -pthread -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR) $(LIB_TARGET) $(TEST_BIN)

-include $(DEPS)
