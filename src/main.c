#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <ping.h>

int			main(int ac, const char **av)
{
	static ping_params	params;
	static ping_stats	stats;
	int					status;
	int					sd;

	status = ping_params_init(&params, ac, av);

	if (status || params.options & OPT_HELP)
		return status;

	sd = ping_socket_init(&params);

	status = -(sd == -1);
	if (status)
	{
		fprintf(stderr, "%s: %s: %s\n", av[0], "socket", strerror(errno));
		return status;
	}

	setlinebuf(stdout);

#if SOCKET_ICMP_USE_DGRAM
	if (params.icmp.socket_type == SOCK_RAW)
#endif
	{
		params.icmp.id = getpid();
	}

	status = ping(sd, &params, &stats);

	if (sd != -1)
	{
		close(sd);
		sd = -1;
	}

	ping_stats_print(&stats);

	return status;
}
