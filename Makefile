CC=gcc
FLAGS= -static-libgcc

all: ipk-server.o ipk-client.o ipk-server ipk-client

ipk-server.o: ipk-server.c msg.h
	$(CC) $(FLAGS) msg.h ipk-server.c -c

ipk-client.o: ipk-client.c msg.h
	$(CC) $(FLAGS) msg.h ipk-client.c -c

ipk-server: ipk-server.o
	$(CC) $(FLAGS) ipk-server.o -o ipk-server

ipk-client: ipk-client.o
	$(CC) $(FLAGS) ipk-client.o -o ipk-client

clean:
	rm *.o ipk-client ipk-server msg.h.gch
