CC = g++
CFLAGS = -std=c++0x

all: clean
	@$(CC) $(CFLAGS) -o G14Project1 src/Project1.cpp src/Sockets.cpp src/packets.cpp src/Gremlin.cpp src/FileManager.cpp src/Client.h src/Server.h

clean:
	@$(RM) G14Project1
