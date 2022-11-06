#define _GNU_SOURCE

#include <sys/socket.h>

#include <stdint.h>
#include <stdio.h>

#include <unistd.h>

#include <arpa/inet.h>

#include <socket_utils.h>
#include <icmp_echo.h>

icmp_echo_stats	stats;

static int	invalid_arguments(const char *name)
{
	fprintf(stderr, "Usage: %s hostname\n", name);
	return 1;
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
		err = icmp_echo(&stats, addr, sd, id, (uint16_t)sequence_i);
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
