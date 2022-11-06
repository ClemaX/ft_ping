#ifndef IP_UTILS_H
# define IP_UTILS_H

#include <netdb.h>
#include <stdint.h>
#include <stddef.h>

int16_t	ip_checksum(const void *data, size_t size);

int		ip_packet();

int		host_address(struct addrinfo **address, const char *node, const char *service);


#endif
