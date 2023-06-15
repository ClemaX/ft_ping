#define _GNU_SOURCE
#include <ip_utils.h>
#include <stdint.h>
#include <stddef.h>

#include <netinet/ip.h>
#include <netdb.h>
#include <stdio.h>

int16_t			ip_checksum(const void *data, size_t size)
{
	int			sum = 0;

	for (size_t i = 0; i < size / 2; i++)
		sum += ((const uint16_t*)data)[i];

	if (size % 2 != 0)
		sum += ((const uint8_t*)data)[size];

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);

	return (int16_t)~sum;
}

int				ip_host_address(struct addrinfo **address,
	const char *node, const char *service)
{
	static const struct addrinfo	hints = {
		.ai_family = AF_INET,
		.ai_flags = 0,
		.ai_socktype = SOCK_RAW
	};

	return getaddrinfo(node, service, &hints, address);
}
