CFLAGS := -c -Wall -Weffc++ -g -std=c++14 -Iinclude
LDFLAGS := -lboost_system -lpthread

# Default target
all: StompEMIClient

# StompEMIClient executable
StompEMIClient: bin/ConnectionHandler.o bin/event.o bin/StompClient.o bin/StompProtocol.o bin/CreateFrames.o
	g++ -o bin/StompEMIClient bin/ConnectionHandler.o bin/event.o bin/StompClient.o bin/StompProtocol.o bin/CreateFrames.o $(LDFLAGS)

# EchoClient executable
EchoClient: bin/ConnectionHandler.o bin/echoClient.o
	g++ -o bin/EchoClient bin/ConnectionHandler.o bin/echoClient.o $(LDFLAGS)

# StompWCIClient executable
StompWCIClient: bin/ConnectionHandler.o bin/StompClient.o bin/event.o bin/CreateFrames.o
	g++ -o bin/StompWCIClient bin/ConnectionHandler.o bin/StompClient.o bin/event.o bin/CreateFrames.o $(LDFLAGS)

# Object files
bin/ConnectionHandler.o: src/ConnectionHandler.cpp
	g++ $(CFLAGS) -o bin/ConnectionHandler.o src/ConnectionHandler.cpp

bin/echoClient.o: src/echoClient.cpp
	g++ $(CFLAGS) -o bin/echoClient.o src/echoClient.cpp

bin/event.o: src/event.cpp
	g++ $(CFLAGS) -o bin/event.o src/event.cpp

bin/StompProtocol.o: src/StompProtocol.cpp
	g++ $(CFLAGS) -o bin/StompProtocol.o src/StompProtocol.cpp

bin/StompClient.o: src/StompClient.cpp
	g++ $(CFLAGS) -o bin/StompClient.o src/StompClient.cpp

bin/CreateFrames.o: src/CreateFrames.cpp
	g++ $(CFLAGS) -o bin/CreateFrames.o src/CreateFrames.cpp

# Clean target
.PHONY: clean
clean:
	rm -f bin/*
