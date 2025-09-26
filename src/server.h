
#ifndef SERVER_H
#define SERVER_H
#include <pthread.h>
#include <netinet/in.h>

// nao implementado nesta etapa 1

typedef struct chat_server chat_server_t;

chat_server_t* chat_server_create(const char* host, int port);
void chat_server_destroy(chat_server_t* s);
int  chat_server_start(chat_server_t* s);
void chat_server_stop(chat_server_t* s);

#endif 
