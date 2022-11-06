#define _GNU_SOURCE

#include <sys/socket.h>

#include <stdint.h>
#include <stdio.h>

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <errno.h>

#include <icmp_packet.h>
#include <socket_utils.h>


typedef struct	ping_stats
{
	unsigned	transmitted;
	unsigned	received;
	char		*host_name;
	char		*host_presentation;
}				ping_stats;

ping_stats	stats;

static int	invalid_arguments(const char *name)
{
	fprintf(stderr, "Usage: %s hostname\n", name);
	return 1;
}

static int	icmp_echo_send(const struct sockaddr_in *addr, int sd,
	uint16_t id, uint16_t sequence)
{
	icmp_packet		*request;
	ssize_t			ret;

	request = icmp_echo_request(addr, id, sequence);
	ret = sendto(sd, request, sizeof(*request), 0, (const struct sockaddr*)addr, sizeof(*addr));

	return -(ret <= 0);
}

static int	icmp_echo_recv(const struct sockaddr_in *addr, int sd,
	struct icmp_packet *request, struct icmp_packet *response)
{
	static uint8_t				control[1024];
	static struct sockaddr_in	src_addr;
	static struct iovec			frames[] =
	{
		{NULL, sizeof(request->ip_header)},
		{NULL, sizeof(request->icmp_header)},
		{NULL, sizeof(request->payload)},
	};
	static struct msghdr		message = {
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

	frames[0].iov_base = &request->ip_header;
	frames[1].iov_base = &request->icmp_header;
	frames[2].iov_base = &request->payload;

	ret = recvmsg(sd, &message, 0);

	err = -(ret != sizeof(*request)
		|| request->icmp_header.type != ICMP_ECHO);

	if (!err)
	{
		frames[0].iov_base = &response->ip_header;
		frames[1].iov_base = &response->icmp_header;
		frames[2].iov_base = &response->payload;

		ret = recvmsg(sd, &message, 0);

		err = -(ret != sizeof(*response)
			|| response->icmp_header.type != ICMP_ECHOREPLY
			|| response->ip_header.saddr != addr->sin_addr.s_addr);
	}

	return err;
}

static int	icmp_echo(const struct sockaddr_in *addr, int sd,
	uint16_t id, uint16_t sequence)
{
	static icmp_packet	request;
	static icmp_packet	response;
	int					err;

	err = icmp_echo_send(addr, sd, id, sequence);
	if (!err)
	{
		fprintf(stderr, "Sent icmp echo request to %s: icmp_seq=%hu\n",
			inet_ntoa(addr->sin_addr), sequence);
		++stats.transmitted;

		err = icmp_echo_recv(addr, sd, &request, &response);
		if (!err)
		{
			fprintf(stderr, "Received icmp echo response from %s: icmp_seq=%hu\n",
				inet_ntoa(addr->sin_addr), ntohs(response.icmp_header.un.echo.sequence));
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

int	ping(const struct sockaddr_in *addr, int sd, uint16_t id, uint64_t count)
{
	int				err;
	uint64_t		sequence_i;

	fprintf(stdout, "PING %s (%s) %zu(%zu) bytes of data.\n",
		stats.host_name, stats.host_presentation,
		sizeof(((icmp_packet*)NULL)->payload), sizeof(icmp_packet));

	sequence_i = 0;
	for (; count == 0 || sequence_i != count; ++sequence_i)
	{
		err = icmp_echo(addr, sd, id, (uint16_t)sequence_i);
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
	int						err;

	if (ac < 2)
		return invalid_arguments(av[0]);

	ret = host_address(&address, av[1], NULL);
	err = ret != 0;
	if (!err)
	{
		sd = socket_raw();

		//setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

		stats.host_name = av[1];
		stats.host_presentation = inet_ntoa(((struct sockaddr_in*)address->ai_addr)->sin_addr);

		err = -(sd == -1);
		if (!err)
		{
			// TODO: Register SIGINT signal() to print stats
			ping((struct sockaddr_in*)address->ai_addr, sd, id, 0);
			err += close(sd);
		}
		freeaddrinfo(address);
	}
	else
		fprintf(stderr, "%s: %s: %s\n", av[0], av[1], gai_strerror(ret));
	return (err);
}
