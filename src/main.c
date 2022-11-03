#define _GNU_SOURCE
#include <sys/socket.h>

#include <stdio.h>

#include <icmp_packet.h>
#include <socket_utils.h>

#include <arpa/inet.h>

#include <netdb.h>
#include <unistd.h>


static int	invalid_arguments(const char *name)
{
	fprintf(stderr, "Usage: %s hostname\n", name);
	return 1;
}

int	ping(const struct sockaddr_in *addr, int sd, int id, unsigned count)
{
	ssize_t		ret;
	unsigned	i;
	icmp_packet	*packet;

	i = 0;
	ret = 0;
	if (count == 0)
	{
		for (;; i++)
		{
			packet = icmp_echo_packet(id, i);
			ret = sendto(sd, packet, sizeof(*packet), 0, addr, sizeof(*addr));
			if (ret > 0)
				fprintf(stderr, "Successfully sent %zd bytes to %s\n", ret, inet_ntoa(addr->sin_addr));
			else
				perror(inet_ntoa(addr->sin_addr));
			usleep(1000 * 1000);
		}
	}
	else
	{
		for (; i != count; i++)
		{
			packet = icmp_echo_packet(id, i);
			ret = sendto(sd, packet, sizeof(*packet), 0, addr, sizeof(*addr));
			if (ret == 0)
			{
				fprintf(stderr, "Successfully sent packet to %s\n", inet_ntoa(addr->sin_addr));
			}
			else
				perror("sendto");
			usleep(1000 * 1000);
		}
	}
	return ret;
}

int	main(int ac, char **av)
{
	const int				id = getpid();
	int						sd;
	struct addrinfo			*address;
	int						ret;

	if (ac < 2)
		return invalid_arguments(av[0]);

	address = host_address(av[1], NULL);
	if (address != NULL)
	{
		sd = socket_raw();
		ret = -(sd == -1);
		if (ret == 0)
		{
			ping((struct sockaddr_in*)address->ai_addr, sd, id, 0);
			ret += close(sd);
		}
		freeaddrinfo(address);
	}
	else
		ret = -1;

	return (ret);
}
