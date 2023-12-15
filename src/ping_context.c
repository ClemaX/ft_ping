#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <ping.h>

#include <libft/opts.h>
#include <socket_utils.h>
#include <time_utils.h>

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


static int	ping_init_params(int ac, const char **av, ping_params *params)
{
	struct addrinfo		*addresses;
	int	ai;
	int	error;

	*params = (ping_params)
	{
		.icmp = {
			.time_to_live = 64,
			.type_of_service = 0,
		},
		.options = 0,
		.interval_s = 1,
	};

	ai = 1;

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

	params->host_name = av[ai];

	error = ip_host_address(&addresses, params->host_name, NULL);
	if (error)
	{
		fprintf(stderr, "%s: %s: %s\n", av[0], params->host_name, gai_strerror(error));
		return error;
	}

	params->icmp.destination = *(struct sockaddr_in*)addresses->ai_addr;
	freeaddrinfo(addresses);

	params->icmp.id = getpid();

	return error;
}

static int	ping_init_socket(ping_params *params)
{
	const struct timeval	timeout = TV_FROM_MS(PING_TIMEOUT_MS);
	const int				on = 1;
	int						sd;

	sd = socket_icmp(&params->icmp.socket_type, params->icmp.time_to_live, params->icmp.type_of_service);

	setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
	setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	setsockopt(sd, SOL_IP, IP_RECVERR, &on, sizeof(on));

	if (params->options & OPT_DEBUG)
		setsockopt(sd, SOL_SOCKET, SO_DEBUG, &on, sizeof(on));
	return sd;
}

int			ping_context_init(int ac, const char **av, ping_context *context) {
	int	error;

	error = ping_init_params(ac, av, &context->params);
	if (error)
		return error;

	context->sd = ping_init_socket(&context->params);

	error = -(context->sd == -1);
	if (error)
	{
		fprintf(stderr, "%s: %s: %s\n", av[0], "socket", strerror(errno));
		return error;
	}

	ping_stats_init(&context->stats, context->params.host_name, &context->params.icmp.destination);

	context->params.icmp.sequence = PING_SEQ_START;

	return error;
}
