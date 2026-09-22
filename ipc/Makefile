CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -Iinclude -MMD -MP
TEST_CXXFLAGS := $(CXXFLAGS) -I../tests/support
AR := ar
ARFLAGS := rcs

BUILD_DIR := build
LIB_TARGET := libdacc-ipc.a
TEST_BIN := tests/bin/test_framed_socket
SOCKET_TEST_BIN := tests/bin/test_local_socket
TEST_SEND_DOUBLE := ../tests/support/send_test_double.cpp
TEST_SEND_DOUBLE_HEADER := ../tests/support/send_test_double.hpp

SRCS := src/FramedSocket.cpp src/LocalSocket.cpp
OBJS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean test

all: $(LIB_TARGET)

$(LIB_TARGET): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_BIN): tests/test_framed_socket.cpp $(TEST_SEND_DOUBLE) $(TEST_SEND_DOUBLE_HEADER) $(LIB_TARGET)
	@mkdir -p $(@D)
	$(CXX) $(TEST_CXXFLAGS) $< $(TEST_SEND_DOUBLE) $(LIB_TARGET) -pthread -Wl,--wrap=send -o $@

$(SOCKET_TEST_BIN): tests/test_local_socket.cpp $(LIB_TARGET)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $< $(LIB_TARGET) -pthread -o $@

test: $(TEST_BIN) $(SOCKET_TEST_BIN)
	./$(TEST_BIN)
	./$(SOCKET_TEST_BIN)

clean:
	rm -rf $(BUILD_DIR) $(LIB_TARGET) $(TEST_BIN) $(SOCKET_TEST_BIN)

-include $(DEPS)
