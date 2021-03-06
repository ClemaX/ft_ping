#ifndef ICMP_PACKET_H
# define ICMP_PACKET_H

# include <netinet/ip.h>
# include <netinet/ip_icmp.h>

# include <ip_utils.h>

typedef struct ip	iphdr;
typedef struct icmp icmphdr;

typedef struct					icmp_packet
{
	iphdr	ip_header;
	icmphdr	icmp_header;
}	__attribute__((__packed__))	icmp_packet;

icmp_packet	*icmp_echo_packet(uint16_t id, uint16_t sequence);

#endif
