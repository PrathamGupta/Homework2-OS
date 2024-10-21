# Define the compiler and the compiler flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall

# Define the target executable and the source files
TARGET = flow
SRCS = flow.cpp

# Define how to build the target executable
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

# Clean the build files
clean:
	rm -f $(TARGET)

# Run the tests
test:
	python run_tests.py

# Phony targets
.PHONY: clean
