-include config.mk
CC=cc -ggdb
CFLAGS= `curl-config --cflags`
LDFLAGS= `curl-config --libs`

all: test_telegram 
	
test_telegram: test_telegram.c libtelegram.a libjsmn.a 
	$(CC) -o test_telegram telegram.c jsmn.c test_telegram.c $(LDFLAGS) $(CFLAGS) -ltelegram -ljsmn -L./
libjsmn.a: jsmn.o
	$(AR) -cvq libjsmn.a jsmn.o

jsmn.o: jsmn.c jsmn.h
	$(CC) -c jsmn.c

telegram.o : telegram.c telegram.h
	$(CC) -c telegram.c $(LDFLAGS) $(CFLAGS)
libtelegram.a: telegram.o
	$(AR) -cvq libtelegram.a telegram.o
clean:
	rm test_telegram
	rm *.o
	rm *.a

