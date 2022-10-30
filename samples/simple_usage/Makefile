CXX = g++
CXXFLAGS = -std=c++17 -Wall -O3 -march=native -I../../user_space_driver
LDFLAGS = -L../../user_space_driver -luser_driver

SOURCES = $(shell echo *.cpp)                                                   
OBJECTS = $(SOURCES:.cpp=.o) 
TARGET = simple_usage

all: $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -rf $(TARGET) $(OBJECTS)
