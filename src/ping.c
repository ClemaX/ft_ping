#include <signal.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>

#include <time_utils.h>
#include <ping.h>

extern	sig_atomic_t interrupt;

static void	ping_response_print(const ping_stats *stats, icmp_packet *response,
	float elapsed_ms)
{
#ifdef PING_STATS_RESPONSE_SHOW_HOSTNAME
	printf(
		"%zu bytes from %s (%s): icmp_seq=%hu ttl=%hu time=%.3lf ms\n",
		sizeof(response.icmp_header) + sizeof(response.payload),
		stats->host_name, stats->host_presentation,
		ntohs(response.icmp_header.un.echo.sequence),
		response.ip_header.ttl,
		elapsed_ms
	);
#else
	printf(
		"%zu bytes from %s: icmp_seq=%hu ttl=%hu time=%.3lf ms\n",
		sizeof(response->icmp_header) + sizeof(response->payload),
		stats->host_presentation,
		ntohs(response->icmp_header.un.echo.sequence),
		response->ip_header.ttl,
		elapsed_ms
	);
#endif
}

static int	ping_error(ping_stats *stats, const struct timeval t[2], int error)
{
	int	unexpected;

	unexpected = 0;

	if (!(error & ICMP_ECHO_ESEND))
	{
		++stats->transmitted;

		stats->time.last_send = t[0];

		if (!(error & ICMP_ECHO_ERECV))
			++stats->received;
		else if (!(error & ICMP_ECHO_ETIMEO))
		{
			if (errno != EINTR)
				perror("recvmsg");
			unexpected = 1;
		}
	}
	else if (!(error & ICMP_ECHO_ETIMEO))
	{
		if (errno != EINTR)
			perror("sendto");
		unexpected = 1;
	}

	return unexpected;
}

int	ping(int sd, ping_stats *stats, icmp_echo_params *params)
{
	static icmp_packet		response;
	static struct timeval	t[2];
	float					elapsed_ms;
	int						error;

	params->destination = stats->destination;

	printf("PING %s (%s) %zu(%zu) bytes of data.\n",
		stats->host_name, stats->host_presentation,
		sizeof(((icmp_packet*)NULL)->payload), sizeof(icmp_packet));

	gettimeofday(&stats->time.start, NULL);

	for (params->sequence = PING_SEQ_START;
		!interrupt
		&& (params->count == 0
			|| params->sequence != params->count + PING_SEQ_START);
		++params->sequence)
	{
		if (params->sequence != PING_SEQ_START && params->interval_s != 0)
			usleep(params->interval_s * 1000 * 1000);

		// TODO: Test last send time with time-outs
		error = icmp_echo(sd, params, &response, t);

		if (!error)
		{
			elapsed_ms = ping_stats_update(stats, t);

			if (!(params->options & OPT_QUIET))
				ping_response_print(stats, &response, elapsed_ms);
		}
		else if (ping_error(stats, t, error))
			break;
	}

	return error;
}
