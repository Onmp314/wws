/* prototypes for wvt util.c */

extern void
_write_utmp (const char *line, const char *user, const char *host, u_long time);

extern int
forkpty(int *master, char *pty, struct termios *t, struct winsize *w);

