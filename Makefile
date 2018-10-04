CC=g++
CFLAGS=-g -Wall -std=gnu++11

all: CS BS user
	@echo Compilation done!

CS: tcp.h tcp.cpp udp.h udp.cpp cserver.cpp
	$(CC) $(CFLAGS) udp.cpp tcp.cpp cserver.cpp -o CS

BS: tcp.h tcp.cpp udp.h udp.cpp bserver.cpp
	$(CC) $(CFLAGS) udp.cpp tcp.cpp bserver.cpp -o BS

user: tcp.h tcp.cpp udp.h udp.cpp user.cpp
	$(CC) $(CFLAGS) udp.cpp tcp.cpp user.cpp -o user

clean:
	rm CS BS user *.txt
