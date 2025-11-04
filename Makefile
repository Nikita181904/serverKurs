CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LIBS = -lcryptopp
TARGET = server

SOURCES = main.cpp server.cpp config.cpp logger.cpp authenticator.cpp processor.cpp network.cpp
HEADERS = server.h config.h logger.h authenticator.h processor.h network.h error_handler.h
OBJECTS = $(SOURCES:.cpp=.o)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

.PHONY: clean install debug