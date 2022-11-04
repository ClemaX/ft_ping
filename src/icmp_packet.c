#include <icmp_packet.h>
#include <netinet/in.h>

icmp_packet	*icmp_echo_request(const struct sockaddr_in *addr, uint16_t id, uint16_t sequence)
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
		.icmp_message =
		{
			.icmp_type = ICMP_ECHO,
			.icmp_code = 0,
		}
	};

	packet.ip_header.daddr = addr->sin_addr.s_addr;

	packet.icmp_message.icmp_data[0] = 42;

	packet.icmp_message.icmp_id = id,
	packet.icmp_message.icmp_seq = sequence,

	packet.icmp_message.icmp_cksum = 0;
	packet.icmp_message.icmp_cksum = ip_checksum(&packet.icmp_message,
		sizeof(packet.icmp_message));

	return &packet;
}
