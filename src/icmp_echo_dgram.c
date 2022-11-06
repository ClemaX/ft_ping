#include <stdio.h>
#include <errno.h>

#include <arpa/inet.h>

#include <icmp_echo.h>


static int	icmp_echo_send(const struct sockaddr_in *addr, int sd,
	uint16_t id, uint16_t sequence)
{
	icmp_packet		*request;
	ssize_t			ret;

	request = icmp_echo_request(addr, id, sequence);

	ret = sendto(sd, &request->icmp_header, sizeof(*request) - sizeof(request->ip_header), 0,
		(const struct sockaddr*)addr, sizeof(*addr));

	return -(ret <= 0);
}

static int	icmp_echo_recv(const struct sockaddr_in *addr, int sd,
	struct icmp_packet *request, struct icmp_packet *response)
{
	static struct sockaddr_in	src_addr;
	static struct iovec			frames[] =
	{
		{NULL, sizeof(request->icmp_header)},
		{NULL, sizeof(request->payload)},
	};
	static uint8_t				control[1024];
	static struct msghdr		message = {
		.msg_name = &src_addr,
		.msg_namelen = sizeof(src_addr),
		.msg_iov = frames,
		.msg_iovlen = sizeof(frames) / sizeof(*frames),
		.msg_control = control,
		.msg_controllen = sizeof(control),
		.msg_flags = 0,
	};
	struct cmsghdr			*cframe;
	ssize_t					ret;
	int						err;
	(void)					addr;

	frames[0].iov_base = &response->icmp_header;
	frames[1].iov_base = &response->payload;

	ret = recvmsg(sd, &message, 0);

	err = -(ret != sizeof(*response) - sizeof(request->ip_header)
		|| response->icmp_header.type != ICMP_ECHOREPLY);

	if (!err)
	{
		for (cframe = CMSG_FIRSTHDR(&message); cframe != NULL;
			cframe = CMSG_NXTHDR(&message, cframe))
		{
			if (cframe->cmsg_level == SOL_IP && cframe->cmsg_type == IP_TTL)
				response->ip_header.ttl = *(int*)CMSG_DATA(cframe);
		}
		// TODO: Retrieve timestamps from SO
	}

	// TODO: Populate or remove request altogether

	return err;
}

int			icmp_echo_dgram(icmp_echo_stats *stats,
	const struct sockaddr_in *addr, int sd, uint16_t id, uint16_t sequence)
{
	static icmp_packet	request;
	static icmp_packet	response;
	int					err;

	err = icmp_echo_send(addr, sd, id, sequence);
	if (!err)
	{
		/*
		fprintf(stderr, "Sent icmp echo request to %s: icmp_seq=%hu\n",
			inet_ntoa(addr->sin_addr), sequence);
		*/
		++(stats->transmitted);

		err = icmp_echo_recv(addr, sd, &request, &response);
		if (!err)
		{
			/*
			fprintf(stderr, "Received icmp echo response from %s: icmp_seq=%hu\n",
				inet_ntoa(addr->sin_addr), ntohs(response.icmp_header.un.echo.sequence));
			*/
			fprintf(stdout, "%zu bytes from %s (%s): icmp_seq=%hu ttl=%hu time=TODO\n",
				sizeof(response.icmp_header) + sizeof(response.payload),
				stats->host_name, stats->host_presentation,
				ntohs(response.icmp_header.un.echo.sequence),
				response.ip_header.ttl);

			++(stats->received);
		}
		else if (errno == EAGAIN || errno == EWOULDBLOCK)
			fprintf(stderr, "Timed out while waiting for response\n");
		else
			perror("recvmsg");
	}
	else
		perror(inet_ntoa(addr->sin_addr));

	return err;
}
