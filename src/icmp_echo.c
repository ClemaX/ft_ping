#include <sys/socket.h>
#include <sys/time.h>

#include <stdio.h>
#include <errno.h>

#include <arpa/inet.h>

#include <socket_utils.h>
#include <icmp_echo.h>
#include <time_utils.h>


static int	icmp_echo_send(const struct sockaddr_in *addr, int sd,
	uint16_t id, uint16_t sequence, struct timeval *timestamp)
{
	icmp_packet	*request;
	ssize_t		ret;

	request = icmp_echo_request(addr, id, sequence);

	gettimeofday(timestamp, NULL);
	fprintf(stderr, "Got t1!\n");

	ret = sendto(sd, request, sizeof(*request), 0,
		(const struct sockaddr*)addr, sizeof(*addr));

	return -(ret <= 0);
}

static int	icmp_echo_recv(const struct sockaddr_in *addr, int sd,
	struct icmp_packet *request, struct icmp_packet *response,
	struct timeval *timestamp)
{
	static struct sockaddr_in	src_addr;
	static struct iovec			frames[] =
	{
		{NULL, sizeof(request->ip_header)},
		{NULL, sizeof(request->icmp_header)},
		{NULL, sizeof(request->payload)},
	};
	struct msghdr *const		message = socket_msghdr(&src_addr, frames,
		sizeof(frames) / sizeof(*frames));
	ssize_t					ret;
	int						err;

	frames[0].iov_base = &request->ip_header;
	frames[1].iov_base = &request->icmp_header;
	frames[2].iov_base = &request->payload;

	ret = recvmsg(sd, message, 0);

	err = -(ret != sizeof(*request)
		|| request->icmp_header.type != ICMP_ECHO);

	if (!err)
	{
		frames[0].iov_base = &response->ip_header;
		frames[1].iov_base = &response->icmp_header;
		frames[2].iov_base = &response->payload;

		ret = recvmsg(sd, message, 0);

		err = -(ret != sizeof(*response)
			|| response->icmp_header.type != ICMP_ECHOREPLY
			|| response->ip_header.saddr != addr->sin_addr.s_addr);

		if (!err)
			icmp_packet_stat(message, timestamp, &response->ip_header.ttl);
	}

	// TODO: Use ctrlmsg to retrieve timestamps from SO

	return err;
}

int			icmp_echo(icmp_echo_stats *stats, const struct sockaddr_in *addr,
	int sd, uint16_t id, uint16_t sequence)
{
	static icmp_packet		request;
	static icmp_packet		response;
	static struct timeval	receive_time;
	float					time;
	int						err;

	err = icmp_echo_send(addr, sd, id, sequence, &stats->last_send_time);
	if (!err)
	{
		/*
		fprintf(stderr, "Sent icmp echo request to %s: icmp_seq=%hu\n",
			inet_ntoa(addr->sin_addr), sequence);
		*/
		++(stats->transmitted);

		err = icmp_echo_recv(addr, sd, &request, &response, &receive_time);
		if (!err)
		{
			/*
			fprintf(stderr, "Received icmp echo response from %s: icmp_seq=%hu\n",
				inet_ntoa(addr->sin_addr), ntohs(response.icmp_header.un.echo.sequence));
			*/
			time = TV_DIFF_MS(stats->last_send_time, receive_time);

			fprintf(stdout, "%zu bytes from %s (%s): icmp_seq=%hu ttl=%hu time=%.1lf ms\n",
				sizeof(response.icmp_header) + sizeof(response.payload),
				stats->host_name, stats->host_presentation,
				ntohs(response.icmp_header.un.echo.sequence),
				response.ip_header.ttl,
				time);

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
