#include <stdio.h>
#include "gpiod.h"

int main(int argc, char *argv[])
{
        int gpio1, gpio2, gpio3;
        int fd;
        int req_fd;
        int value;
        char * file;
	int rv;

	struct gpiod_chip *chip;
	struct gpiod_line *line;

	struct timespec ts = { 0, 1000000 };
	struct gpiod_line_event event;
	struct gpiod_line_request_config config;

        if (argc < 5) {
                printf("Usage: new-gpio-api2 /dev/<gpiochipN> "
                                "<in-gpio> <out-gpio> <event-gpio>\n");
                return -1;
        }

        file = argv[1];

        gpio1 = atoi(argv[2]);
        gpio2 = atoi(argv[3]);
        gpio3 = atoi(argv[4]);

	chip = gpiod_chip_open(file);
	if (!chip)
		return -1;

	/* foo-input */
	line = gpiod_chip_get_line(chip, gpio1);
	if (!line) 
		goto err;

	rv = gpiod_line_request_input(line, "foo-input2");
	if (rv)
		goto err;

	value = gpiod_line_get_value(line);

	/* foo-output */
	line = gpiod_chip_get_line(chip, gpio2);
	if (!line)
		goto err;

	rv = gpiod_line_request_output(line, "foo-output2", 0);
	if (rv)
		goto err;

	gpiod_line_set_value(line, 1);

	line = gpiod_chip_get_line(chip, gpio3);
	if (!line)
		goto err;

	rv = gpiod_line_request_rising_edge_events_flags(line, "foo-event2", 
			GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN);
	if (rv) 
		goto err;

	while (1)
	{
		do {
			rv = gpiod_line_event_wait(line, &ts);
		} while (rv <= 0);

		rv = gpiod_line_event_read(line, &event);
	}

err:
	gpiod_chip_close(chip);

	return -1;
}
