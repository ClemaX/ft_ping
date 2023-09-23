#include <netinet/in.h>
#include <stdio.h>

#include <errno.h>
#include <string.h>

#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <socket_utils.h>
#include <time_utils.h>

#include <libft/opts.h>

#include <ping.h>

static const opt_spec	opt_specs[] =
{
	{
		.short_flag = '?',
		.long_flag = "help",
		.description = "Display this information",
	},
	{
		.short_flag = 'c',
		.long_flag = "count",
		.description = "Stop after sending N packets",
		.parser = opt_parse_count,
	},
	{
		.short_flag = 'd',
		.long_flag = "debug",
		.description = "Set the SO_DEBUG option",
	},
	{
		.short_flag = 'i',
		.long_flag = "interval",
		.description = "Wait N seconds between sending each packet",
		.parser = opt_parse_interval,
	},
	{
		.long_flag = "ttl",
		.description = "Set the IP Time to Live",
		.parser = opt_parse_ttl,
	},
	{
		.short_flag = 'T',
		.long_flag = "tos",
		.description = "Set the IP Type of Service(TOS) to N",
		.parser = opt_parse_tos,
	},
	{
		.short_flag = 'q',
		.long_flag = "quiet",
		.description = "Quiet output",
	},
};

volatile sig_atomic_t	interrupt = 0;

static void handle_interrupt(int signal)
{
	(void)signal;

	interrupt = signal;
}

static int	init_params(int ac, const char **av, icmp_echo_params *params,
	const char **hostname)
{
	int	ai;
	int	error;

	*params = (icmp_echo_params)
	{
		.options = 0,
		.time_to_live = 64,
		.type_of_service = 0,
		.interval_s = 1,
	};

	ai = 1;
	*hostname = NULL;

	params->options = opts_get(opt_specs,
		sizeof(opt_specs) / sizeof(*opt_specs), av, &ai, params);

	error = params->options == OPT_ERROR;
	if (error || params->options & OPT_HELP)
	{
		opts_usage(opt_specs, sizeof(opt_specs) / sizeof(*opt_specs),
			av[0], " hostname");
		return error;
	}

	error = -(ac - ai < 1);
	if (error)
	{
		fprintf(stderr, "%s: missing host operand\n", av[0]);
		opts_usage(opt_specs, sizeof(opt_specs) / sizeof(*opt_specs),
			av[0], " hostname");
		return error;
	}

	*hostname = av[ai];

	params->id = getpid();

	return error;
}

static int	init_socket(icmp_echo_params *params)
{
	const struct timeval	timeout = TV_FROM_MS(PING_TIMEOUT_MS);
	const int				on = 1;
	int						sd;

	sd = socket_icmp(&params->socket_type,
		params->time_to_live, params->type_of_service);

	setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
	setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	if (params->options & OPT_DEBUG)
		setsockopt(sd, SOL_SOCKET, SO_DEBUG, &on, sizeof(on));
	return sd;
}

int			main(int ac, const char **av)
{
	icmp_echo_params		params;
	ping_stats				stats;
	struct sockaddr_in		address;
	struct addrinfo			*addresses;
	int						sd;
	int						error;
	const char				*hostname;

	error = init_params(ac, av, &params, &hostname);
	if (error)
		return error;

	error = ip_host_address(&addresses, hostname, NULL);
	if (error)
	{
		fprintf(stderr, "%s: %s: %s\n", av[0], hostname, gai_strerror(error));
		return error;
	}

	address = *(struct sockaddr_in*)addresses->ai_addr;

	sd = init_socket(&params);

	error = -(sd == -1);
	if (!error)
	{
		signal(SIGINT, handle_interrupt);

		ping_stats_init(&stats, hostname, &address);

		error = ping(sd, &stats, &params);

		error = error && !(error & ICMP_ECHO_ETIMEO) && errno != EINTR;

		if (!error && stats.transmitted != 0)
			ping_stats_print(&stats);

		close(sd);
	}
	else
		fprintf(stderr, "%s: %s: %s\n", av[0], "socket", strerror(errno));

	freeaddrinfo(addresses);

	return (error);
}
