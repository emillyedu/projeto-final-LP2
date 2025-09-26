#ifndef COMMON_H
#define COMMON_H


#include <time.h>
#include <pthread.h>



// Formata a hora local em ISO-8601 com milissegundos e offset de fuso horário
// 'buf' deve ter ao menos 40 bytes
void iso8601_now(char *buf, size_t buflen);


// Converte um pthread_t para unsigned long long para impressão portátil
unsigned long long tid_as_ull(pthread_t tid);


#endif // COMMON_H