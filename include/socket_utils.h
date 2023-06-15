#ifndef SOCKET_UTILS_H
# define SOCKET_UTILS_H

#include <netinet/in.h>

# define SOCKET_ICMP_USE_DGRAM 1

int				socket_icmp(int *socket_type);

struct msghdr	*socket_msghdr(struct sockaddr_in *src_addr,
	struct iovec *frames, size_t frame_count);

#endif
