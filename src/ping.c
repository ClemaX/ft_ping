#include <arpa/inet.h>
#include <signal.h>
#include <stdint.h>
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

static inline void			ping_ip_header_print(
	const icmp_response_packet *response)
{
	const ip_header *const	header = (ip_header*)&response->payload;
	const uint16_t			*header_data = (uint16_t*)header;
	const uint16_t			frag_off = ntohs(header->frag_off);
	char					source_presentation[INET_ADDRSTRLEN];
	char					destination_presentation[INET_ADDRSTRLEN];

	inet_ntop(AF_INET, &header->saddr,
		source_presentation, sizeof(source_presentation));
	inet_ntop(AF_INET, &header->daddr,
		destination_presentation, sizeof(destination_presentation));

	printf(
		"IP Hdr Dump:\n\
 %04hx %04hx %04hx %04hx %04hx %04hx %04hx %04hx %04hx %04hx\n\
Vr "	"HL "		"TOS  "		"Len   "	"ID "		"Flg  "		"off "		"TTL "		"Pro  "		"cks      "	"Src	"	"Dst	Data\n\
%2hhu "	"%2hhu  "	"%02hx "	"%04hx "	"%04hx "	"%3hhu "	"%04hx  "	"%02hhx  "	"%02hhx "	"%04hx "	"%s  "		"%s\n\
",
		ntohs(header_data[0]), ntohs(header_data[1]), ntohs(header_data[2]),
		ntohs(header_data[3]), ntohs(header_data[4]), ntohs(header_data[5]),
		ntohs(header_data[6]), ntohs(header_data[7]), ntohs(header_data[8]),
		ntohs(header_data[9]),
		(uint8_t)header->version, (uint8_t)header->ihl, header->tos,
		ntohs(header->tot_len), ntohs(header->id),
		(uint8_t)((frag_off & ~IP_OFFMASK) >> 13), (uint16_t)(frag_off & IP_OFFMASK),
		header->ttl, header->protocol, ntohs(header->check),
		source_presentation, destination_presentation
	);
}

static inline void			ping_icmp_header_print(
	const icmp_response_packet *response)
{
	const icmp_error_payload *const	payload = &response->payload.error;
	const icmp_header *const		header = (icmp_header*)payload->data;
	const size_t					size =
		ntohs(payload->ip_header.tot_len) - (payload->ip_header.ihl << 2);

	if (header->type == ICMP_ECHO || header->type == ICMP_ECHOREPLY)
	{
		printf(
			"ICMP: type %hhu, code %hhu, size %zu, id 0x%04hx, seq 0x%04hx\n",
			header->type, header->code, size,
			ntohs(header->un.echo.id), ntohs(header->un.echo.sequence)
		);
	}
	else
	{
		printf(
			"ICMP: type %hu, code %hu, size %zu\n",
			header->type, header->code, size
		);
	}
}

static inline void			ping_response_print(
	const icmp_response_packet *response, float elapsed_ms,
	const ping_params *params)
{
	const bool	is_reply = response->icmp_header.type == ICMP_ECHOREPLY;
	char		message_buffer[96];
	const char	*message;

	if (!response->is_valid)
		message = "Invalid checksum";
	else if (is_reply)
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

	printf(
		"%zu bytes from %s: %s\n",
		response->size,
		inet_ntoa((struct in_addr){response->ip_header.saddr}),
		message
	);

	if (!is_reply && (params->options & OPT_VERBOSE))
	{
#if SOCKET_ICMP_USE_DGRAM
		if (params->icmp.socket_type == SOCK_RAW)
#endif
		{
			ping_ip_header_print(response);
		}

		ping_icmp_header_print(response);
	}
}

static inline bool		ping_response_match(
	const icmp_response_packet *response, const ping_params *params,
	uint16_t sequence)
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
	bool								is_reply;

	status = icmp_echo_send(sd, &params->icmp, sequence, &t[0]);

	if (status != 0)
		return status;

	++stats->transmitted;

	do {
		status = icmp_echo_recv(sd, &params->icmp, &response, &t[1]);
	} while (status == 0 && !ping_response_match(&response, params, sequence));

	*elapsed_ms = TV_DIFF_MS(t[0], t[1]);

	if (status == 0)
	{
		is_reply = response.icmp_header.type == ICMP_ECHOREPLY;

		if (response.is_valid && is_reply)
		{
			++stats->received;
			ping_stats_update(stats, *elapsed_ms);
		}

		if (!is_reply || !(params->options & OPT_QUIET))
			ping_response_print(&response, *elapsed_ms, params);
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
