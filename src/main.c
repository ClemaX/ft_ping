#include <netdb.h>
#include <sys/time.h>
#define _GNU_SOURCE

#include <sys/socket.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include <signal.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <socket_utils.h>
#include <icmp_echo.h>
#include <time_utils.h>

#include <ping.h>

icmp_echo_stats	stats;
struct addrinfo	*address;
int				sd;

static int	invalid_arguments(const char *name)
{
	fprintf(stderr, "Usage: %s hostname\n", name);
	return 1;
}

static void handle_interrupt(int signal)
{
	const int	err = signal != SIGINT;

	if (!err)
		printf("\n");

	exit(err);
}

static void	handle_exit()
{
	float	elapsed_ms;
	float	average;
	float	mean_deviation;

	if (sd != -1)
		close(sd);

	if (stats.transmitted != 0)
	{
		elapsed_ms = TV_DIFF_MS(stats.start_time, stats.last_send_time);
		average = stats.time_sum_ms / stats.received;
		mean_deviation = sqrtf(stats.time_sum_ms_sq / stats.received
			- average * average);

		fprintf(stdout,
			"--- %s ping statistics ---\n"
			"%u packets transmitted, %u packets received"
			", %.0f%% packet loss, time %.0lfms\n",
			stats.host_name,
			stats.transmitted, stats.received,
			(stats.transmitted - stats.received) * 100.0 / stats.transmitted,
			elapsed_ms
		);

		if (stats.received != 0)
		{
			fprintf(stdout,
				"rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n",
				stats.min_time_ms,
				average,
				stats.max_time_ms,
				mean_deviation
			);
		}
	}

	if (address != NULL)
		freeaddrinfo(address);
}

static int	ping(const struct sockaddr_in *addr, int sd, int socket_type,
	uint16_t id, uint64_t count)
{
	int				err;
	uint64_t		sequence_i;
	icmp_echo_fun	*echo_fun;

#if SOCKET_ICMP_USE_DGRAM
	if (socket_type == SOCK_RAW)
		echo_fun = icmp_echo;
	else
		echo_fun = icmp_echo_dgram;

#else
	(void)socket_type;
	echo_fun = icmp_echo;
#endif

	stats.min_time_ms = INFINITY;
	stats.max_time_ms = 0;

	stats.time_sum_ms = 0;
	stats.time_sum_ms_sq = 0;

	fprintf(stdout, "PING %s (%s) %zu(%zu) bytes of data.\n",
		stats.host_name, stats.host_presentation,
		sizeof(((icmp_packet*)NULL)->payload), sizeof(icmp_packet));

	gettimeofday(&stats.start_time, NULL);

	for (sequence_i = PING_SEQ_START;
		count == 0 || sequence_i != count - PING_SEQ_START; ++sequence_i)
	{
		err = echo_fun(&stats, addr, sd, id, (uint16_t)sequence_i);

		usleep(1000 * 1000);
	}
	return err;
}

int			main(int ac, char **av)
{
	const int				id = getpid();
	int						sd;
	int						socket_type;
	//struct timeval timeout = {.tv_sec = 2, .tv_usec = 0};
	int						ret;
	int						err;

	if (ac < 2)
		return invalid_arguments(av[0]);

	ret = host_address(&address, av[1], NULL);

	err = ret != 0;
	if (!err)
	{
		sd = socket_icmp(&socket_type);

		// TODO: Set and handle send and receive timeouts
		//setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

		err = -(sd == -1);
		if (!err)
		{
			atexit(handle_exit);
			signal(SIGINT, handle_interrupt);

			stats.host_name = av[1];
			stats.host_presentation =
				inet_ntoa(((struct sockaddr_in*)address->ai_addr)->sin_addr);

			ping((struct sockaddr_in*)address->ai_addr, sd, socket_type, id, 0);
		}
	}
	else
		fprintf(stderr, "%s: %s: %s\n", av[0], av[1], gai_strerror(ret));

	return (err);
}
