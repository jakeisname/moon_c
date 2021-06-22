#ifndef _FOO_H_
#define _FOO_H_

#include <linux/mutex.h>
#include <linux/list.h>

struct foo_info {
	int a;
	int b;
	struct list_head list;
};

extern struct mutex foo_lock;
extern struct list_head foo_list;

#endif /* _FOO_H_ */
