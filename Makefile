CC=g++
CFLAGS=-std=c++11 -m32 -Wall -Llib -Wl,-rpath=lib/ -lhcnetsdk

all:
	$(CC) $(CFLAGS) src/main.cpp -o ivms-bf

clean:
	rm ivms-bf