CC = gcc
CFALGS = -W -Wall
TARGET = tcpd
OBJECTS = peer.o signal.o queue.o stdin.o server.o msg.o option.o main.o

all : $(TARGET)

$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o 
	rm -f $(TARGET)

