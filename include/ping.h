#ifndef PING_H
# define PING_H

# include <sys/time.h>

# include <stdint.h>

# ifndef PING_TIMEOUT_MS
#  define PING_TIMEOUT_MS 1000
# endif

# ifndef PING_SEQ_START
#  define PING_SEQ_START 1
# endif

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


void	ping_stats_init(ping_stats *stats, const char *host_name,
	const struct sockaddr_in *destination);

void	ping_stats_print(const ping_stats *stats);

int		ping(ping_stats *stats, int sd, int socket_type,
	uint16_t id, uint64_t count);

#endif
