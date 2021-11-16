#include <icmp_packet.h>

icmp_packet	*icmp_echo_packet(uint16_t id, uint16_t sequence)
{
	static icmp_packet	packet;

	packet.ip_header.ip_tos = 0;
	packet.ip_header.ip_len = sizeof(icmp_packet);
	// ip_id
	// ip_off
	packet.ip_header.ip_ttl = 64;
	packet.ip_header.ip_p = IPPROTO_ICMP;

	packet.ip_header.ip_sum = 0;

	packet.ip_header.ip_src.s_addr = htonl(INADDR_ANY);
	packet.ip_header.ip_dst.s_addr = htonl(INADDR_LOOPBACK);

	packet.ip_header.ip_sum = ip_checksum(&packet.ip_header,
		sizeof(packet.ip_header));

	packet.icmp_header.icmp_type = ICMP_ECHO;

	packet.icmp_header.icmp_code = 0;
	packet.icmp_header.icmp_cksum = 0;

	packet.icmp_header.icmp_hun.ih_idseq.icd_id = id;
	packet.icmp_header.icmp_hun.ih_idseq.icd_seq = sequence;

	packet.icmp_header.icmp_cksum = ip_checksum(&packet.icmp_header,
		sizeof(packet.icmp_header));

	return &packet;
}
