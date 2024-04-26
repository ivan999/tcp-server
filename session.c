#include "session.h"
#include "dynarr.h"
#include "lexer.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <fcntl.h>
#include <unistd.h>

struct session {
    int sd;
    struct lex_state *lex;
    struct sockaddr_in addr;
};

struct session *make_session(
    int sd, const struct sockaddr_in *addr)
{
    int flags;
    struct session *res = malloc(sizeof(*res));
    res->sd = sd;
    res->addr = *addr;
    flags = fcntl(sd, F_GETFL);
    fcntl(sd, F_SETFL, flags|O_NONBLOCK);
    res->lex = init_lex_state();
    return res;
}

int get_sd_session(const struct session *sess)
{
    return sess->sd;
}

#define SIZEADD_READ 256

static char *read_data(int sd)
{
    int len, size;
    char *res, *ptr;
    INIT_DYNARR(res, len, size);
    for( ; ; ) {
        int curlen;
        RESIZE_DYNARR(res, char, size, SIZEADD_READ);
        ptr = res + len;
        curlen = read(sd, ptr, SIZEADD_READ);
        if(curlen == -1) {
            if(errno != EAGAIN)
                break;
            SIZEMOD_DYNARR(res, char, len, size, SIZEADD_READ);
            res[len] = 0;
            return res;
        }
        if(curlen == 0)
            break;
        len += curlen;
    }
    free(res);
    return NULL;
}

static const char sep_str[] = ": ";
static const char unexp_tok[] = "unexpected token";
static const char uknown_cmd[] = "unknown command";
static const char ending_mess[] = "\n";

#define BUFSIZE 512

/* This function will send a message to the remote host of the
 * session. You can give this function several strings, they will 
 * separate by sep_str variable and end by ending_mess. The last 
 * parameter have to be NULL */
static void send_message(const struct session *sess, ...)
{
    va_list vl;
    const char *mess;
    static char buf[BUFSIZE];
    int len = 0;
    va_start(vl, sess);
    while((mess = va_arg(vl, const char *))) {
        strcpy(buf + len, mess);
        len += strlen(mess);
    }
    strcpy(buf + len, ending_mess);
    len += strlen(ending_mess);
    write(sess->sd, buf, len);
    va_end(vl);
}

/* do_session will call this function when lexer state returns 
 * positive number, what means that a remote host sent you correct
 * full string. */
static void run_command(struct session *sess)
{
    puts("ok");
}

int do_session(struct session *sess) 
{
    int end = 0, len;
    char *data = read_data(sess->sd); 
    if(!data)
        return 0;
    while((len = lexer(sess->lex, data + end))) {
        run_command(sess);
        free_lex_state(sess->lex);
        sess->lex = init_lex_state();
        end += len;
    }
    free(data);
    return 1;
}

void close_session(struct session *sess)
{
    free_lex_state(sess->lex);
    close(sess->sd);
    free(sess);
}
