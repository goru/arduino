CC=gcc
CFLAGS=`pkg-config libusb-1.0 --cflags`
LIBS=`pkg-config libusb-1.0 --libs`

CMD=receive
SRC=receive.c

ifeq ($(shell uname), Linux)
	OS = -D LINUX
else ifeq ($(shell uname), Darwin)
	OS = -D DARWIN
else ifeq ($(shell uname -o), Cygwin)
	OS = -D CYGWIN
endif

all: $(CMD)

$(CMD): $(SRC)
	$(CC) $(SRC) $(CFLAGS) $(LIBS) $(OS) -o $(CMD)

