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
	int			setsockopt_err;

	sd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	if (sd != -1)
	{
		setsockopt_err = setsockopt(sd, IPPROTO_IP, IP_HDRINCL,
			&opt_true, sizeof(opt_true));

		if (!setsockopt_err)
			*socket_type = SOCK_RAW;
	}
	else
	{
#if SOCKET_ICMP_USE_DGRAM
		sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);

		if (sd != -1)
		{
			setsockopt_err =
				setsockopt(sd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl))
			||	setsockopt(sd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos))
			||	setsockopt(sd, IPPROTO_IP, IP_RECVTTL,
					&opt_true, sizeof(opt_true));

			if (!setsockopt_err)
				*socket_type = SOCK_DGRAM;
		}
		else
#endif
			perror("socket");
	}

	if (sd != -1)
	{
		if (!setsockopt_err)
			setsockopt_err = setsockopt(sd, SOL_SOCKET, SO_TIMESTAMP_OLD,
				&opt_true, sizeof(opt_true));

		if (setsockopt_err)
		{
			perror("setsockopt");
			close(sd);
			sd = -1;
		}
	}

	return sd;
}


struct msghdr *socket_msghdr(struct sockaddr_in *src_addr,
	struct iovec *frames, size_t frame_count)
{
	static uint8_t			control[1024];
	static struct msghdr	message = {
		.msg_namelen = sizeof(*src_addr),
		.msg_control = control,
		.msg_controllen = sizeof(control),
		.msg_flags = 0,
	};

	message.msg_name = src_addr;
	message.msg_iov = frames;
	message.msg_iovlen = frame_count;

	return &message;
}

