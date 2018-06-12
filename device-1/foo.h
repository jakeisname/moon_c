
#ifndef __FOO__
#define __FOO__
/**************************************************************************     
 * support foo_device & foo_driver API
 **************************************************************************/    

struct foo_device {
	int match_id;
	char *name;
    struct device dev;                                                
};

struct foo_driver {
	int match_id;
    struct device_driver driver;                                                
    int (*probe)(struct foo_device *);                                     
    int (*remove)(struct foo_device *);                                    
    void (*shutdown)(struct foo_device *);                                 
    int (*suspend)(struct foo_device *, pm_message_t state);               
    int (*resume)(struct foo_device *);                                    
};

#define to_foo_device(x) container_of((x), struct foo_device, dev) 

#define to_foo_driver(drv) (container_of((drv), \
	struct foo_driver, driver)) 


int foo_device_add(struct foo_device *pdev);
void foo_device_del(struct foo_device *pdev);
void foo_device_put(struct foo_device *pdev);                          
int foo_device_register(struct foo_device *pdev);                      
void foo_device_unregister(struct foo_device *pdev);
int foo_driver_register(struct foo_driver *pdev, struct module *owner);
void foo_driver_unregister(struct foo_driver *pdev);
                   
#endif /* __FOO__ */
