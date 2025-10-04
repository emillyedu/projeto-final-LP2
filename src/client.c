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

#define BUF_SZ 2048

#define LOGD(fmt, ...) tslog_log(TSLOG_DEBUG, "client", fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) tslog_log(TSLOG_INFO,  "client", fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) tslog_log(TSLOG_WARN,  "client", fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) tslog_log(TSLOG_ERROR, "client", fmt, ##__VA_ARGS__)

static void usage(const char *p){
    fprintf(stderr,"Uso: %s <host> <porta> [--msg \"texto\"] <logfile>\n", p);
}

int main(int argc, char **argv){
    if(argc<4){ usage(argv[0]); return 1; }

    const char *host=argv[1];
    int port=atoi(argv[2]);
    const char *msg_opt=NULL;
    const char *logpath=NULL;

    for(int i=3;i<argc;i++){
        if(!strcmp(argv[i],"--msg") && i+1<argc){ msg_opt=argv[++i]; }
        else { logpath=argv[i]; }
    }
    if(!logpath){ usage(argv[0]); return 1; }

    if(tslog_init(logpath, /*to_stderr=*/1)!=0){
        fprintf(stderr,"tslog_init falhou para %s\n", logpath); return 1;
    }

    int cfd=socket(AF_INET, SOCK_STREAM, 0);
    if(cfd<0){ LOGE("socket: %s", strerror(errno)); return 1; }

    struct sockaddr_in srv; memset(&srv,0,sizeof(srv));
    srv.sin_family=AF_INET; srv.sin_port=htons(port);
    if(inet_pton(AF_INET, host, &srv.sin_addr)!=1){
        LOGE("Host inválido: %s", host); close(cfd); tslog_flush(); return 1;
    }
    if(connect(cfd,(struct sockaddr*)&srv,sizeof(srv))<0){
        LOGE("connect: %s", strerror(errno)); close(cfd); tslog_flush(); return 1;
    }
    LOGI("Conectado a %s:%d", host, port);

    // Modo script: envia e sai
    if(msg_opt){
        size_t len=strlen(msg_opt);
        if(send(cfd, msg_opt, len, 0)<0) LOGE("send: %s", strerror(errno));
        if(send(cfd, "\n", 1, 0)<0)      LOGE("send NL: %s", strerror(errno));
        close(cfd); tslog_flush(); return 0;
    }

    // Modo interativo (duplica prompt simples)
    fprintf(stdout, "> "); fflush(stdout);

    fd_set rset; char buf[BUF_SZ];

    for(;;){
        FD_ZERO(&rset);
        FD_SET(STDIN_FILENO,&rset);
        FD_SET(cfd,&rset);
        int maxfd=(STDIN_FILENO>cfd?STDIN_FILENO:cfd);

        int nready=select(maxfd+1,&rset,NULL,NULL,NULL);
        if(nready<0){ if(errno==EINTR) continue; LOGE("select: %s", strerror(errno)); break; }

        // servidor → stdout
        if(FD_ISSET(cfd,&rset)){
            ssize_t n=recv(cfd, buf, sizeof(buf)-1, 0);
            if(n<=0){
                if(n<0) LOGW("recv: %s", strerror(errno));
                LOGI("Servidor encerrou a conexão.");
                break;
            }
            buf[n]='\0';
            fputs("\r", stdout);           // volta ao início da linha do prompt
            fputs(buf, stdout);
            fputs("> ", stdout);           // reimprime o prompt
            fflush(stdout);
        }

        // stdin → servidor
        if(FD_ISSET(STDIN_FILENO,&rset)){
            char line[BUF_SZ];
            if(!fgets(line, sizeof(line), stdin)){
                LOGI("EOF no stdin — saindo.");
                break;
            }
            size_t len=strlen(line);
            if(len==0) continue;
            if(send(cfd, line, len, 0)<0){ LOGE("send: %s", strerror(errno)); break; }
            // prompt novamente
            fprintf(stdout, "> "); fflush(stdout);
        }
    }

    close(cfd);
    tslog_flush();
    return 0;
}
