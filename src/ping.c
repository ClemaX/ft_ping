#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <socket_utils.h>
#include <time_utils.h>
#include <ping.h>

static volatile sig_atomic_t	loop_done;

static void					ping_loop_on_interrupt(int signo)
{
	(void)signo;

	if (loop_done)
		return;

	loop_done = true;

	signal(SIGINT, SIG_IGN);
}

static inline void			ping_response_print(const ping_stats *stats,
	const icmp_response_packet *response, float elapsed_ms,
	bool checksum_invalid)
{
	char		message_buffer[96];
	const char	*message;

	if (checksum_invalid)
		message = "Invalid checksum";
	else if (response->icmp_header.type == ICMP_ECHOREPLY)
	{
		snprintf(message_buffer, sizeof(message_buffer),
			"icmp_seq=%hu ttl=%hu time=%.3lf ms",
			ntohs(response->icmp_header.un.echo.sequence),
			response->ip_header.ttl,
			elapsed_ms
		);
		message = message_buffer;
	}
	else
		message = icmp_type_strerror(response->icmp_header.type);

#if PING_STATS_RESPONSE_SHOW_HOSTNAME
	printf(
		"%zu bytes from %s (%s): %s\n",
		response->size,
		stats->host_name,
		inet_ntoa((struct in_addr){response->ip_header.saddr}),
		message
	);
#else
	(void)	stats;
	printf(
		"%zu bytes from %s: %s\n",
		response->size,
		inet_ntoa((struct in_addr){response->ip_header.saddr}),
		message
	);
#endif
}

static inline bool		ping_response_match(const icmp_response_packet *response,
	const ping_params *params, uint16_t sequence)
{
	return response->icmp_header.type != ICMP_ECHO
		&& (response->icmp_header.type != ICMP_ECHOREPLY
			|| ((ntohs(response->icmp_header.un.echo.id) == params->icmp.id
#if SOCKET_ICMP_USE_DGRAM
				|| params->icmp.socket_type == SOCK_DGRAM
#endif
		) && ntohs(response->icmp_header.un.echo.sequence) == sequence));
}

static inline float		ping_loop_on_tick(int sd, const ping_params *params,
	uint16_t sequence, ping_stats *stats, float *elapsed_ms)
{
	static struct icmp_response_packet	response;
	static struct timeval				t[2];
	int									status;

	status = icmp_echo_send(sd, &params->icmp, sequence, &t[0]);

	if (status != 0)
		return status;

	++stats->transmitted;

	do {
		status = icmp_echo_recv(sd, &params->icmp, &response, &t[1]);
	} while (status == 0 && !ping_response_match(&response, params, sequence));

	*elapsed_ms = TV_DIFF_MS(t[0], t[1]);

	if (status == 0 && response.icmp_header.type == ICMP_ECHOREPLY)
	{
		++stats->received;
		ping_stats_update(stats, *elapsed_ms);
	}

	if (!(params->options & OPT_QUIET) && !(status & ~ICMP_ECHO_ECHECKSUM))
		ping_response_print(stats, &response, *elapsed_ms,
			(status & ICMP_ECHO_ECHECKSUM) == ICMP_ECHO_ECHECKSUM);

	return status;
}

static inline int			ping_loop_start(int sd, const ping_params *params,
	ping_stats *stats)
{
	float		elapsed_ms;
	int			status;
	uint16_t	sequence;

	loop_done = 0;

	signal(SIGINT, ping_loop_on_interrupt);

	sequence = PING_SEQ_START;

	while (!loop_done)
	{
		status = ping_loop_on_tick(sd, params, sequence++, stats, &elapsed_ms);

		if (loop_done || status & ICMP_ECHO_EINTR
		|| (params->count != 0 && sequence == params->count + PING_SEQ_START))
			break;

		if (elapsed_ms >= 0 && elapsed_ms < 1000)
			usleep((1000 - elapsed_ms) * 1000);
	}

	return status & ~ICMP_ECHO_EINTR;
}

int							ping_socket_init(ping_params *params)
{
	const struct timeval	timeout = TV_FROM_MS(PING_TIMEOUT_MS);
	const int				on = 1;
	int						sd;

	sd = socket_icmp(&params->icmp.socket_type, params->icmp.time_to_live,
		params->icmp.type_of_service);

	setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
	setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	setsockopt(sd, SOL_IP, IP_RECVERR, &on, sizeof(on));

	if (params->options & OPT_DEBUG)
		setsockopt(sd, SOL_SOCKET, SO_DEBUG, &on, sizeof(on));

	return sd;
}

int							ping(int sd, const ping_params *params,
	ping_stats *stats)
{
	ping_stats_init(stats, params->host_name, &params->icmp.destination);

	printf("PING %s (%s) %zu data bytes\n",
		stats->host_name, stats->host_presentation,
		sizeof(((icmp_echo_packet*)NULL)->payload));

	return ping_loop_start(sd, params, stats);
}
