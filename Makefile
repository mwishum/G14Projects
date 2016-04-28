CC = g++
CFLAGS = -std=c++0x -pthread -D_GLIBCXX_USE_NANOSLEEP

all: clean
	@$(CC) $(CFLAGS) -o G14Project2 src/Project2.cpp src/Sockets.cpp src/packets.cpp src/Gremlin.cpp src/FileManager.cpp

clean:
	@$(RM) G14Project2
