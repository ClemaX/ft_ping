#include <stdio.h>
#include <unistd.h>
#include <string.h>

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
};

int						ping_params_init(ping_params *params, int ac, const char **av)
{
	struct addrinfo	*addresses;
	int				ai;
	int				error;

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

	return error;
}
