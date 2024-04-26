#include "server.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int res;
    long port;
    char *endptr;
    static struct server serv;
    if(argc < 2) {
        fprintf(stderr, "usage <port>\n");
        return 1;
    }
    port = strtol(argv[1], &endptr, 10);
    if(!*argv[1] || *endptr) {
        fprintf(stderr, "invalid port number\n");
        return 2;
    }
    res = init_server(&serv, port);
    if(!res)
        return 3;
    res = do_server(&serv);
    return res ? 0 : 4;
}
