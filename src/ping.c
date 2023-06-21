#include <arpa/inet.h>

#include <signal.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>

#include <unistd.h>

#include <icmp_echo.h>
#include <time_utils.h>
#include <ping.h>

extern	sig_atomic_t interrupt;

void	ping_stats_init(ping_stats *stats, const char *host_name,
	const struct sockaddr_in *destination)
{
	stats->host_name = host_name;
	stats->destination = destination;
	stats->host_presentation = inet_ntoa(destination->sin_addr);

	stats->time.min_ms = INFINITY;
	stats->time.max_ms = 0;

	stats->time.sum_ms = 0;
	stats->time.sum_ms_sq = 0;
}

void	ping_stats_print(const ping_stats *stats)
{
	float	elapsed_ms;
	float	average_ms;
	float	mean_deviation_ms;

	elapsed_ms = TV_DIFF_MS(stats->time.start, stats->time.last_send);
	average_ms = stats->time.sum_ms / stats->received;
	mean_deviation_ms = sqrtf(stats->time.sum_ms_sq / stats->received
		- average_ms * average_ms);

	printf(
		"--- %s ping statistics ---\n"
		"%u packets transmitted, %u received"
		", %.0f%% packet loss, time %.0lfms\n",
		stats->host_name,
		stats->transmitted, stats->received,
		(stats->transmitted - stats->received) * 100.0 / stats->transmitted,
		elapsed_ms
	);

	if (stats->received != 0)
	{
		printf(
			"rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n",
			stats->time.min_ms,
			average_ms,
			stats->time.max_ms,
			mean_deviation_ms
		);
	}
}

int	ping(ping_stats *stats, int sd, int socket_type,
	uint16_t id, uint64_t count)
{
	static icmp_echo_params	params;
	static icmp_packet		response;
	static struct timeval	t[2];
	float					elapsed_ms;
	int						err;

	params.destination = stats->destination;
	params.id = id;

	printf("PING %s (%s) %zu(%zu) bytes of data.\n",
		stats->host_name, stats->host_presentation,
		sizeof(((icmp_packet*)NULL)->payload), sizeof(icmp_packet));

	gettimeofday(&stats->time.start, NULL);

	for (params.sequence = PING_SEQ_START;
		!interrupt
		&& (count == 0 || params.sequence != count - PING_SEQ_START);
		++params.sequence)
	{
		// TODO: Test last send time with time-outs
		err = icmp_echo(sd, socket_type, &params, &response, t);

		if (!err)
		{
			++(stats->transmitted);
			++(stats->received);

			stats->time.last_send = t[0];

			elapsed_ms = TV_DIFF_MS(t[0], t[1]);

			stats->time.sum_ms += elapsed_ms;
			stats->time.sum_ms_sq += elapsed_ms * elapsed_ms;

			if (elapsed_ms < stats->time.min_ms)
				stats->time.min_ms = elapsed_ms;

			if (elapsed_ms > stats->time.max_ms)
				stats->time.max_ms = elapsed_ms;

			printf(
				"%zu bytes from %s (%s): icmp_seq=%hu ttl=%hu time=%.1lf ms\n",
				sizeof(response.icmp_header) + sizeof(response.payload),
				stats->host_name, stats->host_presentation,
				ntohs(response.icmp_header.un.echo.sequence),
				response.ip_header.ttl,
				elapsed_ms
			);
		}
		else
		{
			if (!(err & ICMP_ECHO_ESEND))
			{
				++stats->transmitted;

				stats->time.last_send = t[0];

				if (!(err & ICMP_ECHO_ERECV))
					++stats->received;
				else if (!(err & ICMP_ECHO_ETIMEO))
				{
					if (errno != EINTR)
						perror("recvmsg");
					break;
				}
			}
			else if (!(err & ICMP_ECHO_ETIMEO))
			{
				if (errno != EINTR)
					perror("sendto");
				break;
			}
		}

		usleep(1000 * 1000);
	}

	return err;
}
