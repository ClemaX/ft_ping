#ifndef TIME_UTILS_H
# define TIME_UTILS_H

# define TV_DIFF_MS(t0, t1) (\
	+ ((t1).tv_sec  - (t0).tv_sec) * 1000 \
	+ ((t1).tv_usec - (t0).tv_usec) / 1000.0)

# define TV_FROM_MS(ms) (\
	(struct timeval){\
		.tv_sec = ms / 1000,\
		.tv_usec = ms % 1000\
	} \
)

#endif
