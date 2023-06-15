#include <endian.h>
#include <netinet/ip.h>
#include <stdint.h>
#include <string.h>

#include <netinet/in.h>

#include <icmp_packet.h>

icmp_packet	*icmp_echo_request(const struct sockaddr_in *addr,
	uint16_t id, uint16_t sequence)
{
	static icmp_packet	packet =
	{
		.ip_header =
		{
			.version = 4,
			.ihl = sizeof(packet.ip_header) / 4,
			.tos = 0,
			.tot_len = sizeof(icmp_packet),
			.id = 0,
			.frag_off = 0,
			.ttl = 64,
			.protocol = IPPROTO_ICMP,
		},
		.icmp_header =
		{
			.type = ICMP_ECHO,
			.code = 0,
		},
		.payload = ICMP_ECHO_PAYLOAD,
	};

	packet.ip_header.frag_off = htons(0 | IP_DF);

	packet.ip_header.daddr = addr->sin_addr.s_addr;

	packet.icmp_header.un.echo.id = htons(id);
	packet.icmp_header.un.echo.sequence = htons(sequence);

	packet.icmp_header.checksum = 0;
	packet.icmp_header.checksum = ip_checksum(&packet.icmp_header,
		sizeof(packet.icmp_header) + sizeof(packet.payload));

	return &packet;
}

void	icmp_packet_stat(struct msghdr *message,
	struct timeval *timestamp, uint8_t *ttl)
{
	*timestamp = (struct timeval){0, 0};
	*ttl = 0;

	for (struct cmsghdr *cframe = CMSG_FIRSTHDR(message); cframe != NULL;
		cframe = CMSG_NXTHDR(message, cframe))
	{
		switch (cframe->cmsg_level)
		{
			case SOL_SOCKET:
				if (cframe->cmsg_type == SO_TIMESTAMP_OLD)
					*timestamp = *(struct timeval *)CMSG_DATA(cframe);

				break;

			case SOL_IP:
				if (cframe->cmsg_type == IP_TTL)
					*ttl = *(uint8_t*)CMSG_DATA(cframe);

				break;
		}
	}
}
