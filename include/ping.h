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
#  define PING_SEQ_START 1
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
};

typedef struct	ping_stats
{
	struct
	{
		struct timeval	start;
		struct timeval	last_send;
		float			sum_ms;
		float			sum_ms_sq;
		float			min_ms;
		float			max_ms;
	}							time;
	const struct sockaddr_in	*destination;
	const char					*host_name;
	const char					*host_presentation;
	unsigned					transmitted;
	unsigned					received;
}				ping_stats;

int		ping(int sd, ping_stats *stats, icmp_echo_params *params);

void	ping_stats_init(ping_stats *stats, const char *host_name,
	const struct sockaddr_in *destination);
void	ping_stats_print(const ping_stats *stats);
float	ping_stats_update(ping_stats *stats, const struct timeval t[2]);

const char	*opt_parse_ttl(const char **av, int *ai, void *data);
const char	*opt_parse_tos(const char **av, int *ai, void *data);
const char	*opt_parse_count(const char **av, int *ai, void *data);
const char	*opt_parse_interval(const char **av, int *ai, void *data);

#endif
