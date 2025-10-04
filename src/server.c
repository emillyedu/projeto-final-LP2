// src/server.c
// Servidor TCP multiusuário (broadcast) — integra libtslog da Etapa 1.
// Build:   make
// Run:     ./bin/server 9000 logs/server.log

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#include "tslog.h"  

#define BACKLOG 16
#define BUF_SZ  1024


#define LOGD(fmt, ...) tslog_log(TSLOG_DEBUG, "server", fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) tslog_log(TSLOG_INFO,  "server", fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) tslog_log(TSLOG_WARN,  "server", fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) tslog_log(TSLOG_ERROR, "server", fmt, ##__VA_ARGS__)

static void usage(const char *p) {
    fprintf(stderr, "Uso: %s <porta> <logfile>\n", p);
}

int main(int argc, char **argv) {
    if (argc < 3) { usage(argv[0]); return 1; }
    int port = atoi(argv[1]);
    const char *logpath = argv[2];


    if (tslog_init(logpath, /*to_stderr=*/1) != 0) {
        fprintf(stderr, "tslog_init falhou para %s\n", logpath);
        return 1;
    }

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) { LOGE("socket: %s", strerror(errno)); return 1; }

    int opt = 1; setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr; memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(sfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOGE("bind(%d): %s", port, strerror(errno));
        close(sfd); tslog_flush(); return 1;
    }
    if (listen(sfd, BACKLOG) < 0) {
        LOGE("listen: %s", strerror(errno));
        close(sfd); tslog_flush(); return 1;
    }

    LOGI("Servidor escutando em 0.0.0.0:%d", port);

    int clients[FD_SETSIZE]; for (int i=0; i<FD_SETSIZE; i++) clients[i] = -1;

    fd_set allset, rset;
    FD_ZERO(&allset); FD_SET(sfd, &allset);
    int maxfd = sfd;

    for (;;) {
        rset = allset;
        int nready = select(maxfd+1, &rset, NULL, NULL, NULL);
        if (nready < 0) {
            if (errno == EINTR) continue;
            LOGE("select: %s", strerror(errno));
            break;
        }

        // nova conexão?
        if (FD_ISSET(sfd, &rset)) {
            struct sockaddr_in cli; socklen_t cl = sizeof(cli);
            int cfd = accept(sfd, (struct sockaddr*)&cli, &cl);
            if (cfd < 0) {
                LOGW("accept: %s", strerror(errno));
            } else {
                char ip[INET_ADDRSTRLEN]; inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof(ip));
                LOGI("Conectado: %s:%d (fd=%d)", ip, ntohs(cli.sin_port), cfd);

                int added = 0;
                for (int i=0; i<FD_SETSIZE; i++) {
                    if (clients[i] < 0) {
                        clients[i] = cfd;
                        FD_SET(cfd, &allset);
                        if (cfd > maxfd) maxfd = cfd;
                        added = 1; break;
                    }
                }
                if (!added) {
                    LOGW("Limite de clientes atingido; fechando fd=%d", cfd);
                    close(cfd);
                }
            }
            if (--nready <= 0) continue;
        }

        // dados dos clientes
        for (int i=0; i<FD_SETSIZE; i++) {
            int fd = clients[i];
            if (fd < 0) continue;
            if (FD_ISSET(fd, &rset)) {
                char buf[BUF_SZ];
                ssize_t n = recv(fd, buf, sizeof(buf)-1, 0);
                if (n <= 0) {
                    if (n < 0) LOGW("recv(fd=%d): %s", fd, strerror(errno));
                    LOGI("Desconectou: fd=%d", fd);
                    FD_CLR(fd, &allset);
                    close(fd); clients[i] = -1;
                } else {
                    buf[n] = '\0';
                    LOGD("[fd=%d] %s", fd, buf);
                    // broadcast para todos os outros
                    for (int j=0; j<FD_SETSIZE; j++) {
                        int ofd = clients[j];
                        if (ofd >= 0 && ofd != fd) {
                            if (send(ofd, buf, n, 0) < 0)
                                LOGW("send(fd=%d): %s", ofd, strerror(errno));
                        }
                    }
                }
                if (--nready <= 0) break;
            }
        }
    }

    for (int i=0; i<FD_SETSIZE; i++) if (clients[i] >= 0) close(clients[i]);
    close(sfd);
    tslog_flush();
    return 0;
}
