#ifndef IP_UTILS_H
# define IP_UTILS_H

#include <stdint.h>
#include <stddef.h>

int16_t	ip_checksum(const void *data, size_t size);

int		ip_packet();

struct addrinfo	*host_address(const char *node, const char *service);


#endif
