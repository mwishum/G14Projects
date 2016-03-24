CC = g++
CFLAGS = -std=c++0x -Wall -O

all:
	@$(CC) $(CFLAGS) -o G14Project1 src/Project1.cpp src/Sockets.cpp src/packets.cpp src/Gremlin.cpp \
	 src/FileManager.cpp src/Client.h src/Server.h
    @echo "custom make complete."

clean:
	@$(RM) G14Project1
