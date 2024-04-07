#include "icmp_echo.h"
#include <signal.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>

#include <time_utils.h>
#include <ping.h>

static void	ping_response_print(const ping_stats *stats, icmp_packet *response,
	float elapsed_ms)
{
#ifdef PING_STATS_RESPONSE_SHOW_HOSTNAME
	printf(
		"%zu bytes from %s (%s): icmp_seq=%hu ttl=%hu time=%.3lf ms\n",
		sizeof(response->icmp_header) + sizeof(response->payload),
		stats->host_name, stats->host_presentation,
		ntohs(response->icmp_header.un.echo.sequence),
		response->ip_header.ttl,
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
	int			unexpected;
	const char	*error_message;

	unexpected = 0;
	error_message = NULL;

	if (!(error & ICMP_ECHO_ESEND))
	{
		++stats->transmitted;

		stats->time.last_send = t[0];

		if (!(error & ICMP_ECHO_ERECV))
			++stats->received;
		else if (!(error & ICMP_ECHO_ETIMEO))
		{
			if (error & ICMP_ECHO_EDEST_UNREACH)
				error_message = "Destination unreachable";
			else if (error & ICMP_ECHO_ESOURCE_QUENCH)
				error_message = "Source quench";
			else if (error & ICMP_ECHO_EREDIRECT)
				error_message = "Redirect (change route)";
			else
			{
				if (errno != EINTR)
					perror("recvmsg");

				error_message = "Unexpected error";

				unexpected = 1;
			}
		}
	}
	else if (!(error & ICMP_ECHO_ETIMEO))
	{
		if (errno != EINTR)
			perror("sendto");

		error_message = "Unexpected error";

		unexpected = 1;
	}

	if (error_message != NULL)
	printf("From %s: %s\n", stats->host_presentation, error_message);

	return unexpected;
}

int	ping(int sd, ping_stats *stats, ping_params *params)
{
	static icmp_packet		response;
	static struct timeval	t[2];
	float					elapsed_ms;
	int						error;

	gettimeofday(&stats->time.start, NULL);

	// TODO: Test last send time with time-outs
	error = icmp_echo(sd, &params->icmp, &response, t);

	if (!error)
	{
		elapsed_ms = ping_stats_update(stats, t);

		if (!(params->options & OPT_QUIET))
			ping_response_print(stats, &response, elapsed_ms);
	}
	else if (ping_error(stats, t, error))
		return error;

	return 0;
}
