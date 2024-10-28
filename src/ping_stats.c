#include <arpa/inet.h>

#include <stdio.h>
#include <math.h>

#include <libft/memory.h>

#include <time_utils.h>
#include <ping.h>


void	ping_stats_init(ping_stats *stats, const char *host_name,
	const struct sockaddr_in *destination)
{
	ft_bzero(stats, sizeof(*stats));

	stats->host_name = host_name;
	stats->host_presentation = inet_ntoa(destination->sin_addr);

	stats->time.min_ms = INFINITY;
}

void	ping_stats_print(const ping_stats *stats)
{
	float	average_ms;
	float	mean_deviation_ms;

	average_ms = stats->time.sum_ms / stats->received;
	mean_deviation_ms = sqrtf(stats->time.sum_ms_sq / stats->received
		- average_ms * average_ms);

#if PING_STATS_SHOW_ELAPSED
	float elapsed_ms = TV_DIFF_MS(stats->time.start, stats->time.last_send);

	printf(
		"--- %s ping statistics ---\n"
		"%u packets transmitted, %u received"
		", %.0f%% packet loss, time %.0lfms\n",
		stats->host_name,
		stats->transmitted, stats->received,
		(stats->transmitted - stats->received) * 100.0 / stats->transmitted,
		elapsed_ms
	);
#else
	printf(
		"--- %s ping statistics ---\n"
		"%u packets transmitted, %u received"
		", %.0f%% packet loss\n",
		stats->host_name,
		stats->transmitted, stats->received,
		(stats->transmitted - stats->received) * 100.0 / stats->transmitted
	);
#endif

	if (stats->received != 0)
	{
		printf(
			"round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
			stats->time.min_ms,
			average_ms,
			stats->time.max_ms,
			mean_deviation_ms
		);
	}
}

float	ping_stats_update(ping_stats *stats, const struct timeval t[2])
{
	float					elapsed_ms;

	++(stats->transmitted);
	++(stats->received);

	elapsed_ms = TV_DIFF_MS(t[0], t[1]);

	stats->time.sum_ms += elapsed_ms;
	stats->time.sum_ms_sq += elapsed_ms * elapsed_ms;

	if (elapsed_ms < stats->time.min_ms)
		stats->time.min_ms = elapsed_ms;

	if (elapsed_ms > stats->time.max_ms)
		stats->time.max_ms = elapsed_ms;

	return elapsed_ms;
}
