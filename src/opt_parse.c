#include <limits.h>
#include <errno.h>
#include <stdlib.h>

#include <libft/numbers.h>

#include <ping.h>

const char	*opt_parse_ttl(const char **av, int *ai, size_t param_offset,
	void *data)
{
	ping_params *const	params = (ping_params*)data;
	const char			*end;
	long				value;

	if (av == NULL)
		return "N";

	if (av[*ai] == NULL || av[*ai][param_offset] == '\0')
		return "ttl: a value must be provided";

	value = ft_strtol(av[*ai] + param_offset, &end, 0);

	if (*end != '\0' && value != LONG_MAX && value != LONG_MIN)
		return "ttl: number expected";

	if (value < 1 || value > 255)
		return "ttl: out of range: 1 <= ttl <= 255";

	++(*ai);

	params->icmp.time_to_live = value;

	return NULL;
}

const char	*opt_parse_count(const char **av, int *ai, size_t param_offset,
	void *data)
{
	ping_params *const	params = (ping_params*)data;
	const char			*end;
	long				value;

	if (av == NULL)
		return "N";

	if (av[*ai] == NULL || av[*ai][param_offset] == '\0')
		return "count: a value must be provided";

	value = ft_strtol(av[*ai] + param_offset, &end, 0);

	if (*end != '\0' && errno != ERANGE)
		return "count: number expected";

	if (errno == ERANGE || (unsigned long)value > UINT64_MAX)
		return "count: out of range: LONG_MIN <= count <= UINT64_MAX";

	++(*ai);

	if (value < 0)
		value = 0;

	params->count = (uint64_t)value;

	return NULL;
}

const char	*opt_parse_interval(const char **av, int *ai, size_t param_offset,
	void *data)
{
	ping_params *const	params = (ping_params*)data;
	char				*end;
	float				value;

	if (av == NULL)
		return "N";

	if (av[*ai] == NULL || av[*ai][param_offset] == '\0')
		return "interval: a value must be provided";

	value = strtof(av[*ai] + param_offset, &end);

	if (*end != '\0' && errno != ERANGE)
		return "interval: number expected";

	if (errno == ERANGE || value < 0)
		return "interval: out of range: 0.0 <= interval <= infinity";

	++(*ai);

	params->interval_s = value;

	return NULL;
}

const char	*opt_parse_tos(const char **av, int *ai, size_t param_offset,
	void *data)
{
	ping_params *const	params = (ping_params*)data;
	const char			*end;
	long				value;

	if (av == NULL)
		return "N";

	if (av[*ai] == NULL || av[*ai][param_offset] == '\0')
		return "tos: a value must be provided";

	value = ft_strtol(av[*ai] + param_offset, &end, 0);

	if (*end != '\0' && errno != ERANGE)
		return "tos: number expected";

	if (errno == ERANGE || value < 0 || (unsigned long)value > UCHAR_MAX)
		return "tos: out of range: 0 <= tos <= 255";

	++(*ai);

	params->icmp.type_of_service = (uint64_t)value;

	return NULL;
}
