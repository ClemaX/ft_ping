#include <icmp_packet.h>

#include <arpa/inet.h>

icmp_packet	*icmp_echo_packet(uint16_t id, uint16_t sequence)
{
	static icmp_packet	packet;

	packet.ip_header.check = 0;
	packet.ip_header.daddr = htonl(INADDR_LOOPBACK);
	packet.ip_header.protocol = IPPROTO_ICMP;
	packet.ip_header.saddr = INADDR_ANY;
	packet.ip_header.tos = 0;
	packet.ip_header.tot_len = sizeof(icmp_packet);
	packet.ip_header.ttl = 64;
	packet.ip_header.check = ip_checksum(&packet.ip_header,
		sizeof(packet.ip_header));

	packet.icmp_header.type = ICMP_ECHO;
	packet.icmp_header.code = 0;
	packet.icmp_header.checksum = 0;
	packet.icmp_header.un.echo.id = id;
	packet.icmp_header.un.echo.sequence = sequence;

	packet.icmp_header.checksum = ip_checksum(&packet.icmp_header,
		sizeof(packet.icmp_header));

	return &packet;
}
