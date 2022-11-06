#ifndef ICMP_ECHO_H
# define ICMP_ECHO_H

# include <icmp_packet.h>

typedef struct	icmp_echo_stats
{
	unsigned	transmitted;
	unsigned	received;
	char		*host_name;
	char		*host_presentation;
}				icmp_echo_stats;

typedef int (icmp_echo_fun(icmp_echo_stats *stats,
	const struct sockaddr_in *addr, int sd, uint16_t id, uint16_t sequence));

int			icmp_echo(icmp_echo_stats *stats, const struct sockaddr_in *addr,
	int sd, uint16_t id, uint16_t sequence);

int			icmp_echo_dgram(icmp_echo_stats *stats, const struct sockaddr_in *addr,
	int sd, uint16_t id, uint16_t sequence);

#endif
