#ifndef THREADSAFE_QUEUE_H
#define THREADSAFE_QUEUE_H

#include <pthread.h>
#include <stddef.h>


// Fila limitada (bounded) de ponteiros para char* (mensagens).
typedef struct tsq {
    char      **buf;       // buffer circular de ponteiros (cada um aponta para uma string alocada)
    size_t      cap;       // capacidade total da fila
    size_t      head;      // índice do próximo pop
    size_t      tail;      // índice do próximo push
    size_t      count;     // quantidade de itens na fila
    int         closed;    // se != 0, indica fila fechada (pop retorna NULL quando esvaziar)
    pthread_mutex_t mtx;   // mutex para proteger o estado interno
    pthread_cond_t  can_push; // sinaliza quando é possível enfileirar
    pthread_cond_t  can_pop;  // sinaliza quando é possível desenfileirar
} tsq_t;

// Cria a fila com capacidade 'capacity'; retorna NULL em caso de falha
// As mensagens enfileiradas passam a ser de responsabilidade da fila/consumidor
tsq_t* tsq_create(size_t capacity);

// Fecha a fila: acorda threads esperando e impede novos pushes
void tsq_close(tsq_t* q);

// Destroi a fila. Deve ser chamada após fechar e drenar a fila
void tsq_destroy(tsq_t* q);

// Enfileira a mensagem 'msg'. Em caso de sucesso, a função ASSUME a propriedade de 'msg'
int tsq_push(tsq_t* q, char* msg, int block_ms);

// Desenfileira uma mensagem; bloqueia enquanto estiver vazia, a menos que esteja fechada
char* tsq_pop(tsq_t* q);

#endif 