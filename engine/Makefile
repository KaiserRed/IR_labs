CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
LDFLAGS = -lm

SRC = main.cpp document.cpp tokenizer.cpp stemmer.cpp index.cpp search.cpp zipf.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = engine

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
