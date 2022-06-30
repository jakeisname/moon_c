#include <sys/socket.h>
#include <sys/types.h>
#include <linux/netlink.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>


/* custom netlink protocol family */
#define NETLINK_FOO1_FAMILY 26

#define MAX_PAYLOAD 1024



int create_nl_sock(void)
{
	int sock_fd;
	int ret;
	struct sockaddr_nl src_addr;
	int resource;
	unsigned int buff;
	socklen_t optlen = sizeof(buff);

	sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_FOO1_FAMILY);
	if (sock_fd < 0) {
		printf("open netlink socket failed. err=%d(%s)\n", 
				errno, strerror(errno));
		return -1;
	}

	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = getpid();
	src_addr.nl_groups = 0; //NETLINK_FOO1_FAMILY;

	ret = bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));
	if (ret < 0) {
		printf("bind netlink socket failed. err=%d(%s)\n", 
				errno, strerror(errno));
		return -1;
	}

	return sock_fd;
}

int send_nl_msg(int sock_fd, char *str)
{
	struct sockaddr_nl dest_addr;
	struct nlmsghdr *nlh = NULL;
	struct iovec iov;
	struct msghdr msg;
	int ret;

	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0;		/* to kernel */
	dest_addr.nl_groups = 0;	/* unicast */

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
	nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
	nlh->nlmsg_pid = getpid();
	strcpy(NLMSG_DATA(nlh), str);

	memset(&iov, 0, sizeof(iov));
	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = (void *)&dest_addr;
	msg.msg_namelen = sizeof(dest_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	ret = sendmsg(sock_fd, &msg, 0);

	printf("%s(%d): TX) msg=%s, ret=%d, err=%d(%s)\n",
			__func__, __LINE__, str, 
			ret, errno, strerror(errno));
	free(nlh);

	return ret;
}

int recv_nl_msg(int sock_fd)
{
	struct sockaddr_nl nladdr;
	struct msghdr msg;
	struct iovec iov;
	struct nlmsghdr *nlh;
	int ret;

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
	nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
	nlh->nlmsg_pid = getpid();

	memset(&iov, 0, sizeof(iov));
	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = (void *)&(nladdr);
	msg.msg_namelen = sizeof(nladdr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	ret = recvmsg(sock_fd, &msg, 0);

	printf("%s(%d): RX) pid=%d, msg=%s, ret=%d\n",
			__func__, __LINE__, getpid(), 
			(char *)NLMSG_DATA(nlh), ret);
	free(nlh);

	return ret;
}

int main()
{
	int sock_fd;
	int ret;

	sock_fd = create_nl_sock();
	if (sock_fd < 1)
		return -1;

	ret = send_nl_msg(sock_fd, "Hello?");
	if (ret < 0)
		goto err;

	recv_nl_msg(sock_fd);

err:
	close(sock_fd);

	return 0;
}

