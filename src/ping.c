#include <errno.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <socket_utils.h>
#include <time_utils.h>
#include <ping.h>

static volatile sig_atomic_t	loop_done;

int	ping_socket_init(ping_params *params)
{
	const struct timeval	timeout = TV_FROM_MS(PING_TIMEOUT_MS);
	const int				on = 1;
	int						sd;

	sd = socket_icmp(&params->icmp.socket_type, params->icmp.time_to_live, params->icmp.type_of_service);

	setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
	setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	setsockopt(sd, SOL_IP, IP_RECVERR, &on, sizeof(on));

	if (params->options & OPT_DEBUG)
		setsockopt(sd, SOL_SOCKET, SO_DEBUG, &on, sizeof(on));

	return sd;
}

static void ping_loop_stop()
{
	if (loop_done)
		return;

	loop_done = true;

	signal(SIGINT, SIG_IGN);
}

static void ping_loop_on_interrupt(int signal)
{
	(void)signal;

	ping_loop_stop();
}

static void	ping_response_print(const ping_stats *stats, icmp_packet *response,
	float elapsed_ms)
{
#if PING_STATS_RESPONSE_SHOW_HOSTNAME
	printf(
		"%zu bytes from %s (%s): icmp_seq=%hu ttl=%hu time=%.3lf ms\n",
		sizeof(response->icmp_header) + sizeof(response->payload),
		stats->host_name, inet_ntoa((struct in_addr){response->ip_header.saddr}),
		ntohs(response->icmp_header.un.echo.sequence),
		response->ip_header.ttl,
		elapsed_ms
	);
#else
	(void) stats;
	printf(
		"%zu bytes from %s: icmp_seq=%hu ttl=%hu time=%.3lf ms\n",
		sizeof(response->icmp_header) + sizeof(response->payload),
		stats->host_presentation,
		// inet_ntoa((struct in_addr){response->ip_header.saddr}),
		ntohs(response->icmp_header.un.echo.sequence),
		response->ip_header.ttl,
		elapsed_ms
	);
#endif
}

static int	ping_error(ping_stats *stats, int error)
{
	int			unexpected;
	const char	*error_message;

	unexpected = 0;
	error_message = NULL;

	if (!(error & ICMP_ECHO_ESEND))
	{
		++stats->transmitted;

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
				unexpected = errno != EINTR;
				if (unexpected)
					error_message = "recvmsg";
			}
		}
	}
	else
	{
		unexpected = !(error & ICMP_ECHO_ETIMEO) && errno != EINTR;
		if (unexpected)
			error_message = "sendto";
	}

	if (error_message != NULL)
	{
		if (unexpected)
			printf("%s: %s\n", error_message, strerror(errno));
		else
			printf("From %s: %s\n", stats->host_presentation, error_message);
	}

	return unexpected;
}

static int ping_loop_start(int sd, const ping_params *params, ping_stats *stats)
{
	struct icmp_packet	response;
	struct timeval		t[2];
	float				elapsed_ms;
	int					status;
	uint16_t			sequence;

	loop_done = 0;

	signal(SIGINT, ping_loop_on_interrupt);

	sequence = PING_SEQ_START;
	while (!loop_done)
	{
		bzero(&t[0], sizeof(t[0]));
		status = icmp_echo_send(sd, &params->icmp, sequence, &t[0]);

		if (status == 0)
		{
			bzero(&t[1], sizeof(t[1]));

			do {
				status = icmp_echo_recv(sd, &params->icmp, &response, &t[1]);
			} while (status == 0 && !(
				(params->icmp.socket_type == SOCK_DGRAM
					|| ntohs(response.icmp_header.un.echo.id) == params->icmp.id)
				&& ntohs(response.icmp_header.un.echo.sequence) == sequence));
		}

		elapsed_ms = 0;
		if (status == 0)
		{
			elapsed_ms = ping_stats_update(stats, t);

			if (!(params->options & OPT_QUIET))
				ping_response_print(stats, &response, elapsed_ms);
		}
		else if (ping_error(stats, status))
			break;

		++sequence;

		if (loop_done || (params->count != 0 && sequence == params->count + PING_SEQ_START))
			break;

		usleep(1000 * (1000 - elapsed_ms));
	}

	return status;
}

int ping(int sd, const ping_params *params, ping_stats *stats)
{
	ping_stats_init(stats, params->host_name, &params->icmp.destination);

	printf("PING %s (%s) %zu(%zu) bytes of data.\n",
		stats->host_name, stats->host_presentation,
		sizeof(((icmp_packet*)NULL)->payload), sizeof(icmp_packet));

	return ping_loop_start(sd, params, stats);
}
