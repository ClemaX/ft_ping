#ifndef PING_H
# define PING_H

# include <sys/time.h>

# include <stdint.h>
# include <stdbool.h>

# include <icmp_echo.h>

# ifndef PING_TIMEOUT_MS
#  define PING_TIMEOUT_MS 1000
# endif

# ifndef PING_SEQ_START
#  define PING_SEQ_START 0
# endif

enum options
{
	OPT_NONE = 0,
	OPT_HELP = 1 << 0,
	OPT_COUNT = 1 << 1,
	OPT_DEBUG = 1 << 2,
	OPT_INTERVAL = 1 << 3,
	OPT_TTL = 1 << 4,
	OPT_TOS = 1 << 5,
	OPT_QUIET = 1 << 6,
	OPT_VERBOSE = 1 << 7,
};

typedef struct	ping_stats
{
	struct
	{
		float			sum_ms;
		float			sum_ms_sq;
		float			min_ms;
		float			max_ms;
	}							time;
	const char					*host_name;
	const char					*host_presentation;
	unsigned					transmitted;
	unsigned					received;
}				ping_stats;

typedef struct	ping_params
{
	icmp_echo_params	icmp;
	const char			*host_name;
	uint64_t			count;
	float				interval_s;
	int					options;
}				ping_params;

int			ping(int sd, const ping_params *params, ping_stats *stats);

int			ping_params_init(ping_params *params, int ac, const char **av);
int			ping_socket_init(ping_params *params);

void		ping_stats_init(ping_stats *stats, const char *host_name,
	const struct sockaddr_in *destination);
void		ping_stats_print(const ping_stats *stats);
void		ping_stats_update(ping_stats *stats, float elapsed_ms);

const char	*opt_parse_ttl(const char **av, int *ai, void *data);
const char	*opt_parse_tos(const char **av, int *ai, void *data);
const char	*opt_parse_count(const char **av, int *ai, void *data);
const char	*opt_parse_interval(const char **av, int *ai, void *data);

#endif
