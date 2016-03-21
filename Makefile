-include config.mk
CC=cc
CFLAGS= `curl-config --cflags`

all: libtelegram.a libtelegram.a

test_telegram: libjsmn.a libtelegram.a

libjsmn.a: jsmn.o
	$(AR) -cvq  libjsmn.a jsmn.o
jsmn.o: jsmn.c jsmn.h
	$(CC) -c jsmn.c jsmn.h

libtelegram.a: telegram.o
	$(AR) -cvq  libtelegram.a telegram.o

telegram.o: telegram.h telegram.c
	$(CC) -c telegram.h telegram.c

clean:
	rm *.o
	rm *.a


