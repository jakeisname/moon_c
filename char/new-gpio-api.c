#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/gpio.h>
#include <poll.h>
#include <inttypes.h>
#include <unistd.h>

int chip_info(int fd)
{
	int ret;
	struct gpiochip_info cinfo;

	ret = ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &cinfo);
	if (ret < 0) {
		printf("GPIO_GET_CHIPINFO_IOCTL failed.\n");
		return -1;
	}

	printf("GPIO chip: %s, \"%s\", %u GPIO lines\n", 
		cinfo.name, cinfo.label, cinfo.lines);

	return 0;
}

int line_info(int fd, int gpio)
{
	int ret;
	struct gpioline_info linfo;

	memset(&linfo, 0, sizeof(linfo));
	linfo.line_offset = gpio;

	ret = ioctl(fd, GPIO_GET_LINEINFO_IOCTL, &linfo);
	if (ret < 0) {
		printf("GPIO_GET_LINEINFO_IOCTL failed.\n");
		return -1;
	}

	printf("line %2d: %s\n", linfo.line_offset, linfo.name);

	return fd;
}


int request_gpio(int fd, int gpio, int flags, const char *label, int *req_fd)
{
	struct gpiohandle_request req;
	int ret;

	req.flags = flags;
	req.lines = 1;
	req.lineoffsets[0] = gpio;
	req.default_values[0] = 0;
	strcpy(req.consumer_label, label);

	ret = ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &req);
	if (ret < 0) {
		printf("GPIO_GET_LINEHANDLE_IOCTL failed.\n");
		return -1;
	}

	*req_fd = req.fd;

	return 0;
}

int get_value(int req_fd, int gpio, int *value)
{
	struct gpiohandle_data data;
	int ret;

	memset(&data, 0, sizeof(data));

	ret = ioctl(req_fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
	if (ret < 0) {
		printf("GPIO_GET_LINE_VALUES_IOCTL failed.\n");
		return -1;
	}
	printf("line %d is %s\n", gpio, data.values[0] ? "high" : "low");

	*value = data.values[0];

	return 0;
}

int set_value(int req_fd, int gpio, int value)
{
	struct gpiohandle_data data;
	int ret;

	memset(&data, 0, sizeof(data));
	data.values[0] = value;

	ret = ioctl(req_fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
	if (ret < 0) {
		printf("GPIO_SET_LINE_VALUES_IOCTL failed.\n");
		return -1;
	}
	printf("line %d is %s\n", gpio, data.values[0] ? "high" : "low");

	return 0;
}

int request_event(int fd, int gpio, const char *label, int *req_fd)
{
	struct gpioevent_request req;
	int ret;

	req.lineoffset = gpio;
	strcpy(req.consumer_label, label);
	req.handleflags = GPIOHANDLE_REQUEST_OPEN_DRAIN;
	req.eventflags = GPIOEVENT_REQUEST_RISING_EDGE | 
		GPIOEVENT_REQUEST_FALLING_EDGE;

	ret = ioctl(fd, GPIO_GET_LINEEVENT_IOCTL, &req);
	if (ret < 0) {
		printf("GPIO_GET_LINEEVENT_IOCTL failed.\n");
		return -1;
	}

	*req_fd = req.fd;

	return 0;
}

#define USE_POLL

int recv_event(int req_fd, int gpio)
{
	struct gpioevent_data event;
	struct pollfd pfd;
	ssize_t rd;
	int ret;

#ifdef USE_POLL
	pfd.fd = req_fd;
	pfd.events = POLLIN | POLLPRI;
	
	rd = poll(&pfd, 1, 1000);
	if (rd < 0)
	if (ret < 0) {
		printf("poll failed.\n");
		return -1;
	}
#endif
	ret = read(req_fd, &event, sizeof(event));
	if (ret < 0) {
		printf("read failed.\n");
		return -1;
	}

	printf( "GPIO EVENT @%" PRIu64 ": ", event.timestamp);

	if (event.id == GPIOEVENT_EVENT_RISING_EDGE)
		printf("RISING EDGE");
	else
		printf("FALLING EDGE");
	printf ("\n");

	return 0;
}

int main(int argc, char *argv[])
{
	int gpio1, gpio2, gpio3;
	int fd;
	int req_fd;
	int value;
	char * file;

	if (argc < 5) {
		printf("Usage: new-gpio-api /dev/<gpiochipN> "
				"<in-gpio> <out-gpio> <event-gpio>\n");
		return -1;
	}
	
	file = argv[1];	
	fd = open(file, O_RDWR);
	if (fd < 0) {
		printf("open \"%s\" failed\n", file);
		return -1;
	}
	
	gpio1 = atoi(argv[2]);
	gpio2 = atoi(argv[3]);
	gpio3 = atoi(argv[4]);

	if (chip_info(fd) < 0)
		goto err;

	if (line_info(fd, gpio1) < 0)
		goto err;

	if (request_gpio(fd, gpio1, GPIOHANDLE_REQUEST_INPUT, 
		"foo_input", &req_fd) < 0)
		goto err;

	if (get_value(req_fd, gpio1, &value) < 0)
		goto err;

	if (request_gpio(fd, gpio2, GPIOHANDLE_REQUEST_OUTPUT, 
		"foo_output", &req_fd) < 0)
		goto err;

	if (set_value(req_fd, gpio2, 1) < 0)
		goto err;

	if (request_event(fd, gpio3, "foo_event", &req_fd) < 0)
		goto err;

	while (1)
		if (recv_event(req_fd, gpio3) < 0)
			break;

err:
	close(fd);
	return -1;
}

