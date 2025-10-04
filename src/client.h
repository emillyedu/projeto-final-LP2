#ifndef CLIENT_H
#define CLIENT_H

#include <stdint.h>

#ifndef CLIENT_BUF_SZ
#define CLIENT_BUF_SZ 1024
#endif

#ifdef __cplusplus
extern "C" {
#endif

int client_run(const char *host, uint16_t port,
               const char *msg_opt, const char *logpath);

#ifdef __cplusplus
}
#endif

#endif 
