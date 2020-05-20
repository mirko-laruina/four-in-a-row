/**
 * @file inet_utils.cpp
 * @author Riccardo Mancini
 * 
 * @brief Implementation of inet_utils.h.
 * 
 * @see inet_utils.h
 * 
 * @date 2020-05-17
 */

#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "logging.h"

#include "inet_utils.h"

using namespace std;

int bind_random_port(int socket, struct sockaddr_in *addr){
    int port, ret, i;
    for (i = 0; i < MAX_TRIES; i++){
        if (i == 0) // first I generate a random one
            port = rand() % (TO_PORT - FROM_PORT + 1) + FROM_PORT;
        else //if it's not free I scan the next one
            port = (port - FROM_PORT + 1) % (TO_PORT - FROM_PORT + 1) + FROM_PORT;

        LOG(LOG_DEBUG, "Trying port %d...", port);

        addr->sin_port = htons(port);
        ret = bind(socket, (struct sockaddr *)addr, sizeof(*addr));
        if (ret != -1)
            return port;
        // consider only some errors?
    }
    LOG(LOG_ERR, "Could not bind to random port after %d attempts", MAX_TRIES);
    return 0;
}

struct sockaddr_in make_sv_sockaddr_in(char *ip, int port){
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    return addr;
}

struct sockaddr_in make_my_sockaddr_in(int port){
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    return addr;
}

int sockaddr_in_cmp(struct sockaddr_in sai1, struct sockaddr_in sai2){
    if (sai1.sin_port == sai2.sin_port &&
        sai1.sin_addr.s_addr == sai2.sin_addr.s_addr)
        return 0;
    else
        return 1;
}

string sockaddr_in_to_string(struct sockaddr_in src){
    char dst[MAX_SOCKADDR_STR_LEN];
    char port_str[6];
    const char *ret;

    sprintf(port_str, "%d", ntohs(src.sin_port));

    ret = inet_ntop(AF_INET, (void *)&src.sin_addr, dst, MAX_SOCKADDR_STR_LEN);
    if (ret != NULL){
        strcat(dst, ":");
        strcat(dst, port_str);
    } else {
        strcpy(dst, "ERROR");
    }

    string s = dst;

    return s;
}
