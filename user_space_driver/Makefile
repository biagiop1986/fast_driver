CXX      = g++
CXXFLAGS = -std=c++17 -fPIC -pedantic -Wall -march=native -O3
LDFLAGS  = -shared

TARGET  = libuser_driver.so
SOURCES = $(shell echo *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@

.PHONY: clean
clean:
	rm -rf $(TARGET) $(OBJECTS)
