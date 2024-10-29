#include <errno.h>
#include <signal.h>
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

static inline int			ping_response_validate(const icmp_packet *response)
{
	static const int	status_map[NR_ICMP_TYPES] = {
		[ICMP_DEST_UNREACH] = ICMP_ECHO_EDEST_UNREACH,
		[ICMP_SOURCE_QUENCH] = ICMP_ECHO_EDEST_UNREACH,
		[ICMP_REDIRECT] = ICMP_ECHO_EREDIRECT,
	};
	int	status;

	status = response->icmp_header.type != ICMP_ECHOREPLY;

	if (status != 0)
		status = status_map[response->icmp_header.type];

	return status;
}

static inline void			ping_response_print(const ping_stats *stats,
	const icmp_packet *response, float elapsed_ms)
{
#if PING_STATS_RESPONSE_SHOW_HOSTNAME
	printf(
		"%zu bytes from %s (%s): icmp_seq=%hu ttl=%hu time=%.3lf ms\n",
		sizeof(response->icmp_header) + sizeof(response->payload),
		stats->host_name,
		inet_ntoa((struct in_addr){response->ip_header.saddr}),
		ntohs(response->icmp_header.un.echo.sequence),
		response->ip_header.ttl,
		elapsed_ms
	);
#else
	(void)	stats;
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

static inline const char	*ping_strerror(int status)
{
	const char	*message = NULL;

	if (status != 0 && !(status & ICMP_ECHO_ETIMEO))
	{
		if (status & ICMP_ECHO_EDEST_UNREACH)
			message = "Time to live exceeded";
		else if (status & ICMP_ECHO_ESOURCE_QUENCH)
			message = "Source quench";
		else if (status & ICMP_ECHO_EREDIRECT)
			message = "Redirect (change route)";
		else if (status & ICMP_ECHO_ECHECKSUM)
			message = "Invalid checksum";
		else if (!(status & ICMP_ECHO_EINTR))
			message = "Unexpected error";
	}

	return message;
}

static inline int		ping_recv_error(int error)
{
	int	status;

	if (error == EINTR)
		status = ICMP_ECHO_EINTR;
	else
	{
		status = ICMP_ECHO_ERECV;

		if (error == EAGAIN || error == EWOULDBLOCK)
			status |= ICMP_ECHO_ETIMEO;
		else if (error == ENETDOWN || error == ENETUNREACH || error == ENETRESET
			|| error == EHOSTDOWN || error == EHOSTUNREACH)
			status |= ICMP_ECHO_EDEST_UNREACH;
	}

	return status;
}

static inline bool		ping_response_match(const icmp_packet *response,
	const ping_params *params, uint16_t sequence)
{
	return (ntohs(response->icmp_header.un.echo.id) == params->icmp.id
#if SOCKET_ICMP_USE_DGRAM
		|| params->icmp.socket_type == SOCK_DGRAM
#endif
) && ntohs(response->icmp_header.un.echo.sequence) == sequence;
}

static inline float		ping_loop_on_tick(int sd, const ping_params *params,
	uint16_t sequence, ping_stats *stats, float *elapsed_ms)
{
	static struct icmp_packet	response;
	static struct timeval		t[2];
	int							status;

	status = icmp_echo_send(sd, &params->icmp, sequence, &t[0]);

	if (status != 0)
		return ICMP_ECHO_ESEND;

	++stats->transmitted;

	do {
		status = icmp_echo_recv(sd, &params->icmp, &response, &t[1]);

		if (status == 0)
			status = ping_response_validate(&response);
		else
		{
			gettimeofday(&t[1], NULL);
			status = ping_recv_error(errno);
		}
	} while (status == 0 && !ping_response_match(&response, params, sequence));

	*elapsed_ms = TV_DIFF_MS(t[0], t[1]);

	if (status == 0)
	{
		++stats->received;
		ping_stats_update(stats, *elapsed_ms);

		if (!(params->options & OPT_QUIET))
			ping_response_print(stats, &response, *elapsed_ms);
	}
	else
	{
		const char *message = ping_strerror(status);

		if (message)
		{
			printf("%zu bytes from %s: %s\n",
				sizeof(icmp_packet) - sizeof(ip_header),
				stats->host_presentation, message);
		}
	}

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
		sizeof(((icmp_packet*)NULL)->payload));

	return ping_loop_start(sd, params, stats);
}
