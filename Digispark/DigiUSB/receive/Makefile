CC=g++
CFLAGS=`pkg-config libusb-1.0 --cflags`
LIBS=`pkg-config libusb-1.0 --libs`

CMD=receive
SRC=receive.cpp

all: $(CMD)

$(CMD): $(SRC)
	$(CC) $(CFLAGS) $(LIBS) -o $(CMD) $(SRC)

