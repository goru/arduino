CC=g++
CFLAGS=`pkg-config libusb-1.0 --cflags`
LIBS=`pkg-config libusb-1.0 --libs`

CMD=receive
SRC=receive.cpp

ifeq ($(shell uname), Linux)
	OS = -D LINUX
else ifeq ($(shell uname), Darwin)
	OS = -D DARWIN
endif

all: $(CMD)

$(CMD): $(SRC)
	$(CC) $(CFLAGS) $(LIBS) $(OS) -o $(CMD) $(SRC)

