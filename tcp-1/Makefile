CC = gcc
CFALGS = -W -Wall -g -O0
TARGET = server client
SERVER_OBJECTS = peer.o signal.o queue.o stdin.o server.o msg.o debug.o option.o main.o
CLIENT_OBJECTS = peer.o signal.o queue.o stdin.o client.o msg.o debug.o option.o main2.o

all : $(TARGET)

server: $(SERVER_OBJECTS)
	$(CC) $(CFLAGS) -g -o $@ $^

client: $(CLIENT_OBJECTS)
	$(CC) $(CFLAGS) -g -o $@ $^


.c.o: 
	$(CC) $(CFLAGS) -g -c $< -o $@

clean:
	rm -f *.o 
	rm -f $(TARGET)

