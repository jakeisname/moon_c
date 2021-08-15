
#time ./test; time ./test1; time ./test2; time ./test3

OBJECTS=test.o
OBJECTS1=test1.o
OBJECTS2=test2.o
OBJECTS3=test3.o

all: clean test test1 test2 test3

test: $(OBJECTS)
	@gcc -o test $(OBJECTS) -lpthread

test1: $(OBJECTS1)
	@gcc -o test1 $(OBJECTS1) -lpthread

test2: $(OBJECTS2)
	@gcc -o test2 $(OBJECTS2) -lpthread

test3: $(OBJECTS3)
	@gcc -o test3 $(OBJECTS3) -lpthread

test.o: test.c
	@gcc -c test.c

test1.o: test1.c
	@gcc -c test1.c

test2.o: test2.c
	@gcc -c test2.c

test3.o: test3.c
	@gcc -c test3.c -g -O0

clean:
	@rm -f $(OBJECTS)
	@rm -f $(OBJECTS1)
	@rm -f $(OBJECTS2)
	@rm -f $(OBJECTS3)
	@rm -f test
	@rm -f test1
	@rm -f test2
	@rm -f test3

