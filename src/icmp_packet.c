#include <icmp_packet.h>

icmp_packet	*icmp_echo_packet(uint16_t id, uint16_t sequence)
{
	static icmp_packet	packet;

	packet.ip_header.version = 4;
	packet.ip_header.ihl = sizeof(packet.ip_header) / 4;

	packet.ip_header.tos = 0;
	packet.ip_header.tot_len = sizeof(icmp_packet);
	packet.ip_header.id = 0;
	packet.ip_header.frag_off = 0;
	packet.ip_header.ttl = 64;
	packet.ip_header.protocol = IPPROTO_ICMP;

	packet.ip_header.saddr = htonl(INADDR_ANY);
	packet.ip_header.daddr = htonl(INADDR_LOOPBACK);

	packet.ip_header.check = 0;
	packet.ip_header.check = ip_checksum(&packet.ip_header,
		sizeof(packet.ip_header));

	packet.icmp_message.icmp_type = ICMP_ECHO;

	packet.icmp_message.icmp_code = 0;

	packet.icmp_message.icmp_hun.ih_idseq.icd_id = id;
	packet.icmp_message.icmp_hun.ih_idseq.icd_seq = sequence;

	packet.icmp_message.icmp_cksum = 0;
	packet.icmp_message.icmp_cksum = ip_checksum(&packet.icmp_message,
		sizeof(packet.icmp_message));

	return &packet;
}
