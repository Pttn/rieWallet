CXX    = g++
CFLAGS = -Wall -Wextra -std=c++20
LIBS   = -pthread -lcurl -lcrypto -lfltk

all: standard

standard: CFLAGS += -Os -s
standard: rieWallet

debug: CFLAGS += -g
debug: rieWallet

msys_version := $(if $(findstring Msys, $(shell uname -o)),$(word 1, $(subst ., ,$(shell uname -r))),0)
ifneq ($(msys_version), 0)
static: CFLAGS += -Os -s -D WIN32 -D CURL_STATICLIB -I incs/
static: LIBS   := -static -L libs/ -pthread -lcurl -lssl -lcrypto -lcrypt32 -lfltk -lole32 -luuid -lcomctl32 -lgdi32 -lws2_32 -mwindows -Wl,--image-base -Wl,0x10000000
else
static: CFLAGS += -Os -s -D CURL_STATICLIB -I incs/
static: LIBS   := -Wl,-Bstatic -static-libstdc++ -L libs/ -lfltk -lcurl -lssl -lcrypto -Wl,-Bdynamic -lXft -lfontconfig -lpthread -ldl -lX11
endif
static: rieWallet

rieWallet: main.o bech32.o segwit_addr.o Wallet.o Fltk.o
	$(CXX) $(CFLAGS) -o rieWallet $^ $(LIBS)

Wallet.o: Wallet.cpp
	$(CXX) $(CFLAGS) -c -o Wallet.o Wallet.cpp $(LIBS)

Fltk.o: Gui/Fltk.cpp
	$(CXX) $(CFLAGS) -c -o Fltk.o Gui/Fltk.cpp $(LIBS)

segwit_addr.o: External/segwit_addr.cpp
	$(CXX) $(CFLAGS) -c -o segwit_addr.o External/segwit_addr.cpp $(LIBS)

bech32.o: External/bech32.cpp
	$(CXX) $(CFLAGS) -c -o bech32.o External/bech32.cpp $(LIBS)

main.o: main.cpp
	$(CXX) $(CFLAGS) -c -o main.o main.cpp $(LIBS)

clean:
	rm -rf *.o
