#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <asm-generic/socket.h>
#include <bits/types/struct_iovec.h>
#include <netinet/in.h>
#include <stdint.h>
#include <sys/types.h>
#define _GNU_SOURCE
#include <sys/socket.h>

#include <stdio.h>

#include <icmp_packet.h>
#include <socket_utils.h>

#include <arpa/inet.h>

#include <netdb.h>
#include <unistd.h>

#include <errno.h>

typedef struct	ping_stats
{
	unsigned	transmitted;
	unsigned	received;
}				ping_stats;

ping_stats	stats;

static int	invalid_arguments(const char *name)
{
	fprintf(stderr, "Usage: %s hostname\n", name);
	return 1;
}

static int	icmp_echo_send(const struct sockaddr_in *addr, int sd, int id, int sequence)
{
	icmp_packet		*request;
	ssize_t			ret;

	request = icmp_echo_request(addr, id, sequence);
	ret = sendto(sd, request, sizeof(*request), 0, (const struct sockaddr*)addr, sizeof(*addr));

	return -(ret <= 0);
}

static int	icmp_echo_recv(const struct sockaddr_in *addr, int sd)
{
	static uint8_t				control[1024];
	static struct sockaddr_in	src_addr;
	static icmp_packet			packet;
	static struct iovec			frames[] =
	{
		{&packet.ip_header, sizeof(packet.ip_header)},
		{&packet.icmp_message, sizeof(packet.icmp_message)}
	};
	static struct msghdr	response = {
		.msg_name = &src_addr,
		.msg_namelen = sizeof(src_addr),
		.msg_iov = frames,
		.msg_iovlen = sizeof(frames) / sizeof(*frames),
		.msg_control = control,
		.msg_controllen = sizeof(control),
		.msg_flags = 0,
	};
	ssize_t					ret;
	int						err;

	ret = recvmsg(sd, &response, 0);

	err = -(ret <= 0 || packet.ip_header.saddr != addr->sin_addr.s_addr);

	return err;
}

static int	icmp_echo(const struct sockaddr_in *addr, int sd, int id, int sequence)
{
	int err;

	err = icmp_echo_send(addr, sd, id, sequence);
	if (!err)
	{
		fprintf(stderr, "Sent icmp echo request to %s\n", inet_ntoa(addr->sin_addr));
		++stats.transmitted;

		err = icmp_echo_recv(addr, sd);
		if (!err)
		{
			fprintf(stderr, "Received icmp echo response from %s\n", inet_ntoa(addr->sin_addr));
			++stats.received;
		}
		else if (errno == EAGAIN || errno == EWOULDBLOCK)
			fprintf(stderr, "Timed out while waiting for response\n");
		else
			perror("recvmsg");
	}
	else
		perror(inet_ntoa(addr->sin_addr));

	return err;
}

int	ping(const struct sockaddr_in *addr, int sd, int id, unsigned count)
{
	int				err;
	unsigned		i;

	i = 0;
	for (; count == 0 || i != count; i++)
	{
		err = icmp_echo(addr, sd, id, i);
		usleep(1000 * 1000);
	}
	return err;
}

int	main(int ac, char **av)
{
	const int				id = getpid();
	int						sd;
	struct addrinfo			*address;
	//struct timeval timeout = {.tv_sec = 2, .tv_usec = 0};
	int						ret;

	if (ac < 2)
		return invalid_arguments(av[0]);

	address = host_address(av[1], NULL);
	if (address != NULL)
	{
		sd = socket_raw();

		//setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

		ret = -(sd == -1);
		if (ret == 0)
		{
			// TODO: Register SIGINT signal() to print stats
			ping((struct sockaddr_in*)address->ai_addr, sd, id, 0);
			ret += close(sd);
		}
		freeaddrinfo(address);
	}
	else
		ret = -1;

	return (ret);
}
