LIBGPIOD_DIR=/root/libgpiod/include

all:
	gcc -I. -I$LIBGPIOD_DIR new-gpio-api.c -o new-gpio-api
	gcc -I. -I$LIBGPIOD_DIR -lgpiod new-gpio-api2.c -o new-gpio-api2 

clean:
	rm -f *.o
	rm -f new-gpio-api
	rm -f new-gpio-api2
