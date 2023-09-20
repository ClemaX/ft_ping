#include <stdio.h>

#include <errno.h>
#include <string.h>

#include <signal.h>
#include <unistd.h>

#include <socket_utils.h>
#include <icmp_echo.h>
#include <time_utils.h>

#include <libft/opts.h>

#include <ping.h>

enum option_t
{
	OPT_NONE,
	OPT_HELP,
};

static const t_opt_def	opts_def = {
	.short_opts = (const char[]){
		'h',
		'\0',
	},
	.long_opts = (const char *[]) {
		"help",
		NULL,
	},
	.desc = (const char *[]){
		"Display this information",
		NULL,
	},
	.usage = "Usage: %s [options] hostname\n"
};

volatile sig_atomic_t	interrupt = 0;

static void handle_interrupt(int signal)
{
	(void)signal;

	interrupt = signal;

	printf("\n");
}

int			main(int ac, const char *const *av)
{
	ping_stats				stats;
	const struct timeval	timeout = TV_FROM_MS(PING_TIMEOUT_MS);
	struct addrinfo			*address;
	const int				id = getpid();
	int						sd;
	int						socket_type;
	int						err;
	int						ac_i;
	int						opts;

	ac_i = 1;

	opts = opts_get(&opts_def, &ac_i, av);

	err = opts == OPT_ERROR || (ac - ac_i < 1 && !(opts & OPT_HELP));

	if (err || opts & OPT_HELP)
	{
		opts_usage(&opts_def, av[0]);
		return err;
	}

	err = ip_host_address(&address, av[1], NULL);

	if (!err)
	{
		sd = socket_icmp(&socket_type);

		setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
		setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

		err = -(sd == -1);
		if (!err)
		{
			signal(SIGINT, handle_interrupt);

			ping_stats_init(&stats, av[1],
				(struct sockaddr_in*)address->ai_addr);

			err = ping(&stats, sd, socket_type, id, 0);

			err = err && !(err & ICMP_ECHO_ETIMEO) && errno != EINTR;

			fflush(stderr);

			if (!err && stats.transmitted != 0)
				ping_stats_print(&stats);

			close(sd);
		}
		else
			fprintf(stderr, "%s: %s: %s\n", av[0], "socket", strerror(errno));

		freeaddrinfo(address);
	}
	else
		fprintf(stderr, "%s: %s: %s\n", av[0], av[1], gai_strerror(err));

	return (err);
}
