#include <stdio.h>

#include <errno.h>
#include <string.h>

#include <signal.h>
#include <unistd.h>

#include <socket_utils.h>
#include <icmp_echo.h>
#include <time_utils.h>

#include <ping.h>

volatile sig_atomic_t	interrupt = 0;

static int	invalid_arguments(const char *name)
{
	fprintf(stderr, "Usage: %s hostname\n", name);

	return 1;
}

static void handle_interrupt(int signal)
{
	(void)signal;

	interrupt = signal;

	printf("\n");
}

int			main(int ac, char **av)
{
	ping_stats				stats;
	const struct timeval	timeout = TV_FROM_MS(PING_TIMEOUT_MS);
	struct addrinfo			*address;
	const int				id = getpid();
	int						sd;
	int						socket_type;
	int						err;

	if (ac < 2)
		return invalid_arguments(av[0]);

	err = ip_host_address(&address, av[1], NULL);

	if (!err)
	{
		sd = socket_icmp(&socket_type);

		setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
		setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

		err = -(sd == -1);
		if (!err)
		{
			signal(SIGINT, handle_interrupt);

			ping_stats_init(&stats, av[1],
				(struct sockaddr_in*)address->ai_addr);

			err = ping(&stats, sd, socket_type, id, 0);

			err = err && !(err & ICMP_ECHO_ETIMEO) && errno != EINTR;

			fflush(stderr);

			if (!err && stats.transmitted != 0)
				ping_stats_print(&stats);

			close(sd);
		}
		else
			fprintf(stderr, "%s: %s: %s\n", av[0], "socket", strerror(errno));

		freeaddrinfo(address);
	}
	else
		fprintf(stderr, "%s: %s: %s\n", av[0], av[1], gai_strerror(err));

	return (err);
}
