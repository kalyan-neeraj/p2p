# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic

# OpenSSL libraries
LIBS = -lssl -lcrypto

# Executable name
TARGET = ./tracker

# Source files
SRCS = main.cpp tracker.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Header
HDRS = $(wildcard *.h)

# Default rule
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

# Rule to build object files
%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJS) $(TARGET)

# Installation

# Phony targets
.PHONY: all clean install
