# Compilador e flags
CXX      := g++
CXXFLAGS := -std=c++17 
LDFLAGS  :=

# Diretórios
BINDIR   := bin
LOGDIR   := logs

# Fontes por executável
MAIN_SRCS   := main.cpp \
                GameManager.cpp \
                $(LOGDIR)/LogClient.cpp \
                $(LOGDIR)/LogManager.cpp

LOGGER_SRCS := $(LOGDIR)/LogServer.cpp \
                $(LOGDIR)/LogManager.cpp

# Headers (para dependências)
MAIN_HDRS   := GameManager.hpp \
               $(LOGDIR)/LogClient.hpp \
               $(LOGDIR)/LogManager.hpp

LOGGER_HDRS := $(LOGDIR)/LogManager.hpp

.PHONY: all clean

all: $(BINDIR)/main $(BINDIR)/logger

# --------------------------------------------------
# Target: bin/main
# --------------------------------------------------
$(BINDIR)/main: $(MAIN_SRCS) $(MAIN_HDRS)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $(MAIN_SRCS) -o $@ $(LDFLAGS)

# --------------------------------------------------
# Target: bin/logger
# --------------------------------------------------
$(BINDIR)/logger: $(LOGGER_SRCS) $(LOGGER_HDRS)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $(LOGGER_SRCS) -o $@ $(LDFLAGS)

# --------------------------------------------------
# Limpeza
# --------------------------------------------------
clean:
	rm -rf $(BINDIR)
