# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic

# Executable name
TARGET = client

# Source files
SRCS = client_handler.cpp parser.cpp user.cpp  # Include all necessary source files

# Object files
OBJS = $(SRCS:.cpp=.o)

# Header files
HDRS = $(wildcard *.h)

# OpenSSL libraries
LIBS = -lssl -lcrypto

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

# Phony targets
.PHONY: all clean
