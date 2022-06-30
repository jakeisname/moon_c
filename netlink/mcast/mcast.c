
#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <net/net_namespace.h>


/* custom netlink protocol family */
#define NETLINK_FOO2_FAMILY 27

/* custom netlink multicast group */
#define NETLINK_FOO2_GROUP   2


static struct task_struct *foo_th;
struct sock *nl_socket = NULL;


int send_nl_mcast(const char *msg)
{
	size_t message_size;
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	int ret;

	message_size = strlen(msg) + 1;

	skb = nlmsg_new(NLMSG_ALIGN(message_size), GFP_KERNEL);
	if (!skb) {
		printk("Failed to allocate a new skb\n");
		return -1;
	}

	nlh = nlmsg_put(skb, 0, 1, NLMSG_DONE, message_size, 0);
	strncpy(nlmsg_data(nlh), msg, message_size);

	ret = nlmsg_multicast(nl_socket, skb, 0, NETLINK_FOO2_GROUP, 0);

	printk(KERN_INFO"%s(%d): TX) msg=%s, ret=%d\n", 
			__func__, __LINE__, msg, ret);

	return 0;
}


static int create_nl_sock(void)
{
	struct netlink_kernel_cfg nl_cfg = { };

	nl_socket = netlink_kernel_create(&init_net, NETLINK_FOO2_FAMILY, &nl_cfg);
	if (!nl_socket) {
		printk(KERN_ALERT "Error creating netlik socket.\n");
		return -1;
	}

	return 0;
}


static int foo_thread(void *data)
{
	char buff[100];
	int i = 0;

	printk("foo thread running\n");

	do {
		sprintf(buff, "mcast-msg-%d", ++i);

		send_nl_mcast(buff);

		msleep(3000);

	} while (!kthread_should_stop());

	printk("foo thread stopped\n");

	return 0;
}


static int __init mcast_k_init(void) 
{
	int ret;

	printk("%s(%d):\n", __func__, __LINE__);

	ret = create_nl_sock();
	if (ret)
		return -1;

	foo_th = kthread_run(foo_thread, 0, "foo");

	printk("%s(%d): foo thread created\n", 
			__func__, __LINE__);

	return 0;
}

static void __exit mcast_k_exit(void) {

	printk(KERN_INFO "%s(%d):\n", __func__, __LINE__);

	kthread_stop(foo_th);

	if (nl_socket)
		netlink_kernel_release(nl_socket);
}

module_init(mcast_k_init); 
module_exit(mcast_k_exit);

MODULE_LICENSE("GPL");

