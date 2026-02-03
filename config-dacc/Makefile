CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -I.
AR := ar
ARFLAGS := rcs

BIN_DIR := ../bin
LIB_TARGET := libdacc-config.a
BIN_TARGET := $(BIN_DIR)/dacc-config

SRCS_LIB := HardwareControl.cpp
OBJS_LIB := $(SRCS_LIB:.cpp=.o)

SRCS_BIN := main.cpp
OBJS_BIN := $(SRCS_BIN:.cpp=.o)

.PHONY: all clean

all: $(LIB_TARGET)

$(LIB_TARGET): $(OBJS_LIB)
	$(AR) $(ARFLAGS) $@ $^

$(BIN_TARGET): $(OBJS_BIN) $(LIB_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJS_BIN) -L. -ldacc-config -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o $(LIB_TARGET) $(BIN_TARGET)
