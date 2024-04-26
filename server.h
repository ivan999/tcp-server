#ifndef SERVER_H_SENTRY
#define SERVER_H_SENTRY

/* For stoping this server you can use 
 * Ctrl-C, this will send a signal to the process
 * and it will close all conections, free allocated
 * memory and return control to the calling function */

struct server {
    int ls;
    struct qsessions *qsess;
};

int init_server(struct server *serv, unsigned short port);

int do_server(struct server *serv);

#endif
