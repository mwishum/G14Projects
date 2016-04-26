CC = g++
CFLAGS = -std=c++0x

all: clean
	@$(CC) $(CFLAGS) -o G14Project2 src/Project2.cpp src/Sockets.cpp src/packets.cpp src/Gremlin.cpp src/FileManager.cpp src/side/Client.h
	src/side/Server.h

clean:
	@$(RM) G14Project1
