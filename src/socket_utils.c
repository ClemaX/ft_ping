#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include <netinet/ip.h>

#include <socket_utils.h>

int	socket_icmp(int *socket_type)
{
	const int	opt_true = 1;
#if SOCKET_ICMP_USE_DGRAM
	const int	ttl = 64;
	const int	tos = 0;
#endif
	int			sd;
	sd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	if (sd != -1)
	{
		if (setsockopt(sd, IPPROTO_IP, IP_HDRINCL,
			&opt_true, sizeof(opt_true)) == 0)
		{
			*socket_type = SOCK_RAW;
		}
		else
		{
			perror("setsockopt");
			close(sd);
			sd = -1;
		}
	}
	else
	{
#if SOCKET_ICMP_USE_DGRAM
		sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);

		if (sd != -1)
		{
			if (setsockopt(sd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) != 0
			|| setsockopt(sd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos)) != 0
			|| setsockopt(sd, IPPROTO_IP, IP_RECVTTL, &opt_true, sizeof(opt_true)))
				perror("setsockopt");
			*socket_type = SOCK_DGRAM;
		}
		else
#endif
			perror("socket");
	}
	return sd;
}
