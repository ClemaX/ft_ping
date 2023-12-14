#include <stdlib.h>
#include <stdio.h>

#include <errno.h>

#include <signal.h>
#include <unistd.h>

#include <ping.h>

static ping_context				context;
static volatile sig_atomic_t	loop_done;

static void loop_stop() {
	if (loop_done)
		return;

	loop_done = true;

	signal(SIGALRM, SIG_DFL);
	signal(SIGINT, SIG_IGN);

	if (context.sd != -1) {
		close(context.sd);
		context.sd = -1;
	}

	if (!context.error) {
		printf("\n");
		ping_stats_print(&context.stats);
	}

	exit(context.error);
}

static void	loop_on_tick(int signal)
{
	(void)signal;
	int	error;

	error = ping(context.sd, &context.stats, &context.params);

	context.error = error && !(error & ICMP_ECHO_ETIMEO) && errno != EINTR;

	if (context.error || (context.params.count != 0 && ++context.params.icmp.sequence == context.params.count + PING_SEQ_START))
		loop_stop();
	else
		alarm(context.params.interval_s);
}

static void loop_on_interrupt(int signal)
{
	(void)signal;

	loop_stop();
}

static void	loop_start() {
	loop_done = 0;

	signal(SIGALRM, loop_on_tick);
	signal(SIGINT, loop_on_interrupt);

	loop_on_tick(0);

	while (true)
		usleep(context.params.interval_s * 1000 * 1000);
}

int			main(int ac, const char **av)
{
	int	error;

	error = ping_context_init(ac, av, &context);

	if (error)
		return error;

	printf("PING %s (%s) %zu(%zu) bytes of data.\n",
		context.stats.host_name, context.stats.host_presentation,
		sizeof(((icmp_packet*)NULL)->payload), sizeof(icmp_packet));

	loop_start();

	return -1;
}
