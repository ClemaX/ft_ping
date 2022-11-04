#ifndef ICMP_PACKET_H
# define ICMP_PACKET_H

# include <netinet/ip.h>
# include <netinet/ip_icmp.h>

# include <ip_utils.h>

typedef struct iphdr	ip_header;
typedef struct icmp		icmp_message;

typedef struct					icmp_packet
{
	ip_header		ip_header;
	icmp_message	icmp_message;
}	__attribute__((__packed__))	icmp_packet;

icmp_packet	*icmp_echo_request(uint16_t id, uint16_t sequence);

#endif
