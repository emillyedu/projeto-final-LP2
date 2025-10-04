#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include "tslog.h"

#define MAX_CLIENTS   FD_SETSIZE
#define BACKLOG       32
#define BUF_SZ        2048
#define NICK_MAX      32
#define MSG_MAX       1024

// Macros de log c/ módulo fixo
#define LOGD(fmt, ...) tslog_log(TSLOG_DEBUG, "server", fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) tslog_log(TSLOG_INFO,  "server", fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) tslog_log(TSLOG_WARN,  "server", fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) tslog_log(TSLOG_ERROR, "server", fmt, ##__VA_ARGS__)

typedef struct {
    int  fd;
    char nick[NICK_MAX];
} client_t;

static void usage(const char *p){ fprintf(stderr, "Uso: %s <porta> <logfile>\n", p); }

static void trim_newline(char *s){
    if(!s) return;
    size_t n=strlen(s);
    while(n>0 && (s[n-1]=='\n' || s[n-1]=='\r')) s[--n]='\0';
}

static void nick_sanitize(char *s){
    // mantém [A-Za-z0-9_-], troca outros por '_', limita tamanho
    for(size_t i=0; s[i]; ++i){
        if(!(isalnum((unsigned char)s[i]) || s[i]=='_' || s[i]=='-')) s[i]='_';
    }
    s[NICK_MAX-1] = '\0';
}

static void send_line(int fd, const char *line){
    if(!line) return;
    size_t len = strlen(line);
    if(send(fd, line, len, 0) < 0){ /* ignore erro de send no helper */ }
}

static void sendf(int fd, const char *fmt, ...){
    char buf[MSG_MAX+64];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    send_line(fd, buf);
}

static void broadcast(client_t *clients, int except_fd, const char *line){
    for(int i=0;i<MAX_CLIENTS;i++){
        if(clients[i].fd>=0 && clients[i].fd!=except_fd){
            send_line(clients[i].fd, line);
        }
    }
}

static int find_slot(client_t *clients){
    for(int i=0;i<MAX_CLIENTS;i++) if(clients[i].fd<0) return i;
    return -1;
}

static const char* nick_or_default(const client_t *c){
    return (c->nick[0]? c->nick : "anon");
}

int main(int argc, char **argv){
    if(argc<3){ usage(argv[0]); return 1; }
    int port = atoi(argv[1]);
    const char *logpath = argv[2];

    if(tslog_init(logpath, /*to_stderr=*/1)!=0){
        fprintf(stderr,"tslog_init falhou para %s\n", logpath);
        return 1;
    }

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd<0){ LOGE("socket: %s", strerror(errno)); return 1; }
    int opt=1; setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr; memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET; addr.sin_addr.s_addr=htonl(INADDR_ANY); addr.sin_port=htons(port);

    if(bind(sfd,(struct sockaddr*)&addr,sizeof(addr))<0){
        LOGE("bind(%d): %s", port, strerror(errno)); close(sfd); tslog_flush(); return 1;
    }
    if(listen(sfd, BACKLOG)<0){
        LOGE("listen: %s", strerror(errno)); close(sfd); tslog_flush(); return 1;
    }
    LOGI("Servidor escutando em 0.0.0.0:%d", port);

    client_t clients[MAX_CLIENTS];
    for(int i=0;i<MAX_CLIENTS;i++){ clients[i].fd=-1; clients[i].nick[0]='\0'; }

    fd_set allset, rset;
    FD_ZERO(&allset); FD_SET(sfd,&allset);
    int maxfd=sfd;

    // Mensagem de boas-vindas padrão
    const char *motd = "Welcome! Comandos: /nick NOME, /list, /quit\n";

    for(;;){
        rset=allset;
        int nready=select(maxfd+1,&rset,NULL,NULL,NULL);
        if(nready<0){ if(errno==EINTR) continue; LOGE("select: %s", strerror(errno)); break; }

        // conexões novas
        if(FD_ISSET(sfd,&rset)){
            struct sockaddr_in cli; socklen_t cl=sizeof(cli);
            int cfd=accept(sfd,(struct sockaddr*)&cli,&cl);
            if(cfd<0){
                LOGW("accept: %s", strerror(errno));
            }else{
                int slot = find_slot(clients);
                if(slot<0){
                    LOGW("Limite de clientes; recusando fd=%d", cfd);
                    send_line(cfd, "Servidor cheio. Tente mais tarde.\n");
                    close(cfd);
                }else{
                    clients[slot].fd=cfd;
                    clients[slot].nick[0]='\0';
                    FD_SET(cfd,&allset);
                    if(cfd>maxfd) maxfd=cfd;

                    char ip[INET_ADDRSTRLEN]; inet_ntop(AF_INET,&cli.sin_addr,ip,sizeof(ip));
                    LOGI("Conectado: %s:%d (fd=%d, slot=%d)", ip, ntohs(cli.sin_port), cfd, slot);
                    send_line(cfd, motd);

                    char joinmsg[128];
                    snprintf(joinmsg,sizeof(joinmsg),"* Um novo cliente entrou (fd=%d)\n", cfd);
                    broadcast(clients, cfd, joinmsg);
                }
            }
            if(--nready<=0) continue;
        }

        // dados dos clientes
        for(int i=0;i<MAX_CLIENTS;i++){
            int fd = clients[i].fd;
            if(fd<0) continue;
            if(!FD_ISSET(fd,&rset)) continue;

            char buf[BUF_SZ];
            ssize_t n=recv(fd, buf, sizeof(buf)-1, 0);
            if(n<=0){
                if(n<0) LOGW("recv(fd=%d): %s", fd, strerror(errno));
                LOGI("Saindo: fd=%d nick=%s", fd, nick_or_default(&clients[i]));
                char part[128];
                snprintf(part,sizeof(part),"* %s saiu\n", nick_or_default(&clients[i]));
                broadcast(clients, fd, part);

                FD_CLR(fd,&allset);
                close(fd);
                clients[i].fd=-1;
                clients[i].nick[0]='\0';
            }else{
                buf[n]='\0';
                // processar por linhas (mensagens podem vir agrupadas)
                char *saveptr=NULL;
                for(char *line=strtok_r(buf,"\n",&saveptr); line; line=strtok_r(NULL,"\n",&saveptr)){
                    trim_newline(line);
                    if(line[0]=='\0') continue;

                    // comandos
                    if(line[0]=='/'){
                        if(!strncmp(line,"/nick ",6)){
                            char want[NICK_MAX]; memset(want,0,sizeof(want));
                            strncpy(want, line+6, NICK_MAX-1);
                            trim_newline(want); nick_sanitize(want);
                            if(want[0]=='\0'){
                                send_line(fd, "uso: /nick NOME\n");
                            }else{
                                char old[NICK_MAX];
                                snprintf(old, sizeof(old), "%s", nick_or_default(&clients[i]));
                                snprintf(clients[i].nick, sizeof(clients[i].nick), "%s", want);


                                char ok[96]; snprintf(ok,sizeof(ok),"* nick agora é '%s'\n", clients[i].nick);
                                send_line(fd, ok);

                                char announce[160];
                                snprintf(announce,sizeof(announce),"* %s agora é %s\n", old, clients[i].nick);
                                broadcast(clients, fd, announce);
                                LOGI("fd=%d mudou nick: %s -> %s", fd, old, clients[i].nick);
                            }
                        }else if(!strcmp(line,"/list")){
                            send_line(fd, "* usuários:\n");
                            for(int k=0;k<MAX_CLIENTS;k++){
                                if(clients[k].fd>=0){
                                    sendf(fd, "  - %s (fd=%d)\n", nick_or_default(&clients[k]), clients[k].fd);
                                }
                            }
                        }else if(!strcmp(line,"/quit")){
                            // simulamos fechamento do peer
                            LOGI("quit solicitado por fd=%d (%s)", fd, nick_or_default(&clients[i]));
                            send_line(fd, "* bye\n");
                            // força desconexão: simulamos n<=0 no próximo laço
                            shutdown(fd, SHUT_RDWR);
                        }else{
                            send_line(fd, "comandos: /nick NOME | /list | /quit\n");
                        }
                        continue;
                    }

                    // mensagem normal → broadcast com prefixo de nick
                    char out[MSG_MAX+64];
                    snprintf(out, sizeof(out), "[%s] %s\n", nick_or_default(&clients[i]), line);
                    LOGD("msg fd=%d nick=%s: %s", fd, nick_or_default(&clients[i]), line);
                    broadcast(clients, fd, out);
                }
            }

            if(--nready<=0) break;
        }
    }

    // encerra
    for(int i=0;i<MAX_CLIENTS;i++) if(clients[i].fd>=0) close(clients[i].fd);
    close(sfd);
    tslog_flush();
    return 0;
}
