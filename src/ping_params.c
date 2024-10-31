#include <unistd.h>

#include <ping.h>

#include <libft/opts.h>

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
	{
		.short_flag = 'v',
		.long_flag = "verbose",
		.description = "Verbose output",
	},
};

int						ping_params_perror(const char *name, const char *message)
{
	fprintf(stderr, "%s: %s\n", name, message);
		opts_usage(opt_specs, sizeof(opt_specs) / sizeof(*opt_specs),
			name, " hostname");

	return OPT_ERROR;
}

int						ping_params_init(ping_params *params, const char **av,
	int *opt_end)
{
	int	status;

	*params = (ping_params)
	{
		.icmp = {
			.time_to_live = 64,
			.type_of_service = 0,
		},
		.options = 0,
		.interval_s = 1,
	};

	*opt_end = 1;

	params->options = opts_get(opt_specs,
		sizeof(opt_specs) / sizeof(*opt_specs), av, opt_end, params);

	status = params->options == OPT_ERROR;
	if (status || params->options & OPT_HELP)
	{
		opts_usage(opt_specs, sizeof(opt_specs) / sizeof(*opt_specs),
			av[0], " hostname");
		return status;
	}


#if SOCKET_ICMP_USE_DGRAM
	if (params.icmp.socket_type == SOCK_RAW)
#endif
	{
		params->icmp.id = getpid();
	}

	return status;
}

int					ping_params_args_next(int ai, const char **av)
{
	return args_next(opt_specs, sizeof(opt_specs) / sizeof(*opt_specs),
		ai, av);
}
