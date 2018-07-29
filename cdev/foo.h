#ifndef __FOO_H__
#define __FOO_H__

/* 
 * _IOR(), _IOW(), _IORW() make 32bit cmd
 *
 * 	- 1st argument: type(this example use magic number 0xB4)
 * 	- 2nd argument: command number range is 0x00 ~ 0xff
 * 	- 3nd argument: data type for data size
 *
 * cmd format:
 * 	- bit[31:30]	direction(bit[30]=read, bit[31]=write)
 * 	- bit[29:16]	data size
 * 	- bit[15:8]	type(magic number)
 * 	- bit[7:0]	number(command number)
 */
#define IOCTL_FOO_GET_A 	_IOR(0xB4, 0x01, struct foo_data)
#define IOCTL_FOO_SET_A         _IOW(0xB4, 0x02, struct foo_data)

struct foo_data {                                                                  
    int a;                                                                  
    int b;                                                                  
    char buff[128];                                                         
};

#endif /* __FOO_H__ */

