CC=g++
LIB=-lhcnetsdk
SILENT_LIB=-lsilenthcnetsdk
LIB_DIR=lib/Linux
CFLAGS=-std=c++11 -m32 -Wall -L$(LIB_DIR) -Wl,-rpath=$(LIB_DIR)

all:
	$(CC) $(CFLAGS) $(SILENT_LIB) src/main.cpp -o ivms-bf

vanilla:
	$(CC) $(CFLAGS) $(LIB) src/main.cpp -o ivms-bf

windows:
	i686-w64-mingw32-g++ -std=c++11 -m32 -Wall -Llib/Windows -Wl,-rpath=lib/Windows -lHCNetSDK src/main.cpp -o ivms-bf.exe

clean:
	rm ivms-bf