#include "server.h"
#include "session.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/select.h>

struct item_sess {
    struct session *sess;
    struct item_sess *next;
};

struct qsessions {
    struct item_sess *first, *last;
};

#define LISTEN_QLEN 16

static const char socket_err[] = "creation socket";
static const char bind_err[] = "binding socket";
static const char listen_err[] = "listening mode";
static const char select_err[] = "selection";
static const char accept_err[] = "acception client";

static void sigint_handler(int sig)
{
    signal(SIGINT, sigint_handler);
}

static struct item_sess *get_null_item()
{
    struct item_sess *res = malloc(sizeof(struct item_sess));
    res->sess = NULL;
    res->next = NULL;
    return res;
}

static void init_qsess(struct qsessions *qsess)
{
    qsess->first = get_null_item();
    qsess->last = qsess->first;
}

static void put_qsess(
    struct qsessions *qsess, int sd, const struct sockaddr_in *addr)
{
    qsess->last->sess = make_session(sd, addr);
    qsess->last->next = get_null_item();
    qsess->last = qsess->last->next;
}

static void delete_qsess(
    struct qsessions *qsess, struct item_sess **pp)
{
    struct item_sess *item = *pp;
    *pp = item->next;
    if(item == qsess->first)
        qsess->first = item->next;
    close_session(item->sess);
    free(item);
}

#define ITEM_ISNT_NULL(ITEM) ((ITEM)->sess)

static void clear_qsess(struct qsessions *qsess)
{
    struct item_sess **pp = &qsess->first;
    while(ITEM_ISNT_NULL(*pp)) 
        delete_qsess(qsess, pp);
    free(*pp);
}

int init_server(struct server *serv, unsigned short port)
{
    int res, opt = 1;
    static struct sockaddr_in addr;
    serv->ls = socket(AF_INET, SOCK_STREAM, 0);
    if(serv->ls == -1) {
        perror(socket_err);
        return 0;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    setsockopt(serv->ls, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    res = bind(serv->ls, (struct sockaddr*)&addr, sizeof(addr));
    if(res == -1) {
        perror(bind_err);
        return 0;
    }
    res = listen(serv->ls, LISTEN_QLEN);
    if(res == -1) {
        perror(listen_err);
        return 0;
    }
    serv->qsess = malloc(sizeof(struct qsessions));
    init_qsess(serv->qsess);
    return 1;
}

static void fill_readfds(
    fd_set *readfds, int *maxfd, const struct server *serv)
{
    struct item_sess *item;
    FD_ZERO(readfds);
    FD_SET(serv->ls, readfds);
    *maxfd = serv->ls;
    item = serv->qsess->first;
    while(ITEM_ISNT_NULL(item)) {
        int sd = get_sd_session(item->sess);
        FD_SET(sd, readfds);
        if(sd > *maxfd)
            *maxfd = sd;
        item = item->next;
    }
}

static void accept_client(struct server *serv)
{
    static struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int sd = accept(serv->ls, (struct sockaddr*)&addr, &len);
    if(sd == -1) {
        perror(accept_err);
        return;
    }
    put_qsess(serv->qsess, sd, &addr);
}

static void service_clients(struct server *serv, const fd_set *readfds)
{
    struct item_sess **pp;
    if(FD_ISSET(serv->ls, readfds))
        accept_client(serv); 
    pp = &serv->qsess->first;
    while(ITEM_ISNT_NULL(*pp)) {
        int res = 1;
        int sd = get_sd_session((*pp)->sess);
        if(FD_ISSET(sd, readfds))
            res = do_session((*pp)->sess);
        if(res)
            pp = &(*pp)->next;
        else
            delete_qsess(serv->qsess, pp);
    }
}

int do_server(struct server *serv)
{ 
    int res;
    sigset_t new, old;
    sigemptyset(&new);
    sigaddset(&new, SIGINT);
    signal(SIGINT, sigint_handler);
    sigprocmask(SIG_BLOCK, &new, &old);
    for( ; ; ) {
        int maxfd;
        fd_set readfds;
        fill_readfds(&readfds, &maxfd, serv);
        res = pselect(maxfd+1, &readfds, NULL, NULL, NULL, &old);
        if(res == -1) {
            res = 0;
            if(errno == EINTR)
                res = 1;
            else
                perror(select_err);
            break;
        }
        service_clients(serv, &readfds);
    }
    sigprocmask(SIG_BLOCK, &old, NULL);
    clear_qsess(serv->qsess);
    free(serv->qsess);
    close(serv->ls);
    return res;
}
