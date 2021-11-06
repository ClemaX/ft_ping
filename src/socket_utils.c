#include <netinet/ip.h>
#include <stdio.h>
#include <unistd.h>

#include <socket_utils.h>

int	socket_raw()
{
	const int	include_header = 1;
	int			sd = socket(AF_INET, SOCK_RAW, SOCK_RAW);

	if (sd != -1)
	{
		if (setsockopt(sd, IPPROTO_IP, IP_HDRINCL,
			&include_header, sizeof(include_header)) != 0)
		{
			perror("setsockopt");
			close(sd);
			sd = -1;
		}
	}
	else
		perror("socket");
	return sd;
}
