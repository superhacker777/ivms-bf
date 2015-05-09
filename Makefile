CC=g++
LIB=-lhcnetsdk
SILENT_LIB=-lsilenthcnetsdk
LIB_DIR=lib/Linux
CFLAGS=-std=c++11 -m32 -Wall -L$(LIB_DIR) -Wl,-rpath=$(LIB_DIR)

all:
	$(CC) $(CFLAGS) -O2 src/main.cpp $(SILENT_LIB) -o ivms-bf
	strip ivms-bf

vanilla:
	$(CC) $(CFLAGS) $(LIB) src/main.cpp -o ivms-bf

windows:
	i686-w64-mingw32-g++ -std=c++11 -m32 -march=native -Werror -static-libgcc -static-libstdc++ src/main.cpp -Llib/Windows -Wl,-rpath=lib/Windows -lHCNetSDK -o ivms-bf.exe -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic

clean:
	rm ivms-bf
