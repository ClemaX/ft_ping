#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <ping.h>

#include <libft/opts.h>

static inline int	resolve(
	struct sockaddr_in *address, const char *host_name)
{
	struct addrinfo	*addresses;
	const int		status = ip_host_address(&addresses, host_name, NULL);

	if (status == 0)
	{
		*address = *(struct sockaddr_in*)addresses->ai_addr;
		freeaddrinfo(addresses);
	}

	return status;
}

static inline int	resolve_perror(const char *name, int error)
{
	const char	*message;

	if (error == EAI_NONAME)
		message = "unknown host";
	else
		message = gai_strerror(error);

	fprintf(stderr, "%s: %s\n", name, message);

	return error;
}

int					main(int ac, const char **av)
{
	(void)				ac;
	static ping_params	params;
	static ping_stats	stats;
	int					status;
	int					sd;
	int					opt_end;
	int					arg_i;

	status = ping_params_init(&params, av, &opt_end);

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

	arg_i = ping_params_args_next(0, av);

	if (arg_i == -1)
		return ping_params_perror(av[0], "missing host operand");

	do
	{
		params.host_name = av[arg_i];

		if (resolve(&params.destination, params.host_name) == 0)
			status |= ping(sd, &params, &stats);
		else
			resolve_perror(av[0], status);

		arg_i = ping_params_args_next(arg_i, av);
	}
	while (arg_i != ARG_END);

	if (sd != -1)
	{
		close(sd);
		sd = -1;
	}

	if (status == 0)
		status = stats.received == 0;

	return status;
}
