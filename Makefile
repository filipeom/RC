CC=g++
CFLAGS=-g -Wall -std=gnu++11

all: CS BS user
	@echo Compilation done!

CS: udp.o tcp.o common.o cserver.cpp 
	$(CC) $(CFLAGS)  cserver.cpp udp.o tcp.o common.o -o CS

BS: tcp.o udp.o common.o bserver.cpp
	$(CC) $(CFLAGS)  bserver.cpp udp.o tcp.o common.o -o BS

user: tcp.h udp.h common.h user.cpp
	$(CC) $(CFLAGS)  user.cpp udp.o tcp.o common.o -o user

udp.o: udp.h udp.cpp
	$(CC) $(CFLAGS) -c udp.cpp -o udp.o

tcp.o: tcp.h tcp.cpp
	$(CC) $(CFLAGS) -c tcp.cpp -o tcp.o

common.o: common.h common.cpp
	$(CC) $(CFLAGS) -c common.cpp -o common.o

clean:
	rm CS BS user *.txt *.o

trash:
	rm *.txt

zip:
	zip proj_43.zip *.cpp *.h Makefile README.md
