
#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <net/net_namespace.h>


/* custom netlink protocol family */
#define NETLINK_FOO1_FAMILY 26


struct sock *nl_socket = NULL;
static int _pid;


int send_nl_ucast(const char *msg)
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

	ret = nlmsg_unicast(nl_socket, skb, _pid);

	printk(KERN_INFO"%s(%d): TX) pid=%d, msg=%s, ret=%d\n", 
			__func__, __LINE__, _pid, msg, ret);

	return 0;
}

static void recv_nl_ucast(struct sk_buff *skb) 
{
	struct nlmsghdr *nlh;

	nlh = (struct nlmsghdr*)skb->data;
	_pid = nlh->nlmsg_pid;

	printk(KERN_INFO "%s(%d): RX) pid=%d, msg=%s\n", 
			__func__, __LINE__, _pid, (char*)nlmsg_data(nlh));

	send_nl_ucast("Hello!");
}

static int create_nl_sock(void)
{
	struct netlink_kernel_cfg nl_cfg = {
		.input = recv_nl_ucast,
	};

	nl_socket = netlink_kernel_create(&init_net, NETLINK_FOO1_FAMILY, &nl_cfg);
	if (!nl_socket) {
		printk(KERN_ALERT "Error creating netlik socket.\n");
		return -1;
	}

	return 0;
}

static int __init ucast_init(void) 
{
	int ret;

	printk("%s(%d):\n", __func__, __LINE__);

	ret = create_nl_sock();

	return ret;
}

static void __exit ucast_exit(void) {

	printk(KERN_INFO "%s(%d):\n", __func__, __LINE__);

	if (nl_socket)
		netlink_kernel_release(nl_socket);
}

module_init(ucast_init); 
module_exit(ucast_exit);

MODULE_LICENSE("GPL");

