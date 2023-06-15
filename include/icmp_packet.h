#ifndef ICMP_PACKET_H
# define ICMP_PACKET_H

# include <stdint.h>

# include <netinet/ip.h>
# include <netinet/ip_icmp.h>

# include <ip_utils.h>

# define ICMP_ECHO_PAYLOAD \
	"********************************************************"

typedef struct iphdr	ip_header;
typedef struct icmphdr	icmp_header;

typedef struct					icmp_packet
{
	ip_header	ip_header;
	icmp_header	icmp_header;
	uint8_t		payload[56];
}	__attribute__((__packed__))	icmp_packet;

icmp_packet		*icmp_echo_request(const struct sockaddr_in *addr,
	uint16_t id, uint16_t sequence);

#endif
