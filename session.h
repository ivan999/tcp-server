#ifndef SESSION_H_SENTRY
#define SESSION_H_SENTRY

/* There are functions what server will call for 
 * servicing its connections.
 * You can change file session.c for making your 
 * own parameters for session */

#include <netinet/in.h>

struct session *make_session(
    int sd, const struct sockaddr_in *addr);

int get_sd_session(const struct session *sess);

int do_session(struct session *sess);

void close_session(struct session *sess);

#endif
