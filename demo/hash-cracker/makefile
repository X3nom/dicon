CC = gcc
CFLAGS = -Wall \
	-I../../diconlib/include -I../../common/include \
	-L../../diconlib/lib -ldiconlib \
	-lm -g -lcrypto -lpthread


SRC = main.c


all: main shared

main:
	$(CC) $(SRC) $(CFLAGS) -o find-primes-main 
	
shared:
	$(CC) $(SRC) $(CFLAGS) -fPIC -shared -static-libgcc -o find-primes.so


clean:
	rm ./find-primes-main
	rm ./find-primes.so