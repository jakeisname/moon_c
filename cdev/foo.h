#ifndef __FOO_H__
#define __FOO_H__

#define IOCTL_FOO_GET_A         1                                               
#define IOCTL_FOO_SET_A         2                                               

struct foo_data {                                                                  
    int a;                                                                  
    int b;                                                                  
    char buff[128];                                                         
};

#endif /* __FOO_H__ */

