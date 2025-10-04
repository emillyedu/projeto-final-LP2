#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>

#ifndef SERVER_BACKLOG
#define SERVER_BACKLOG 16
#endif

#ifndef SERVER_BUF_SZ
#define SERVER_BUF_SZ 1024
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Executa o loop principal do servidor.
 * Retorna 0 em sucesso; !=0 em erro.
 */
int server_run(uint16_t port, const char *logpath);

#ifdef __cplusplus
}
#endif

#endif 
