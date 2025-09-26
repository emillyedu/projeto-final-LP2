#include "tslog.h"
#include "threadsafe_queue.h"
#include "common.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#ifndef TSLOG_QUEUE_CAP
#define TSLOG_QUEUE_CAP 8192
#endif

#ifndef TSLOG_PUSH_TIMEOUT_MS
#define TSLOG_PUSH_TIMEOUT_MS 50
#endif

// Contexto global do logger (estado interno)
typedef struct {
    FILE   *fp;         // arquivo de log (pode ser NULL)
    int     to_stderr;  //!= 0, também loga para stderr
    int     level;      // nível mínimo de log
    tsq_t  *q;          // fila threadsafe de mensagens
    pthread_t writer;
    int     running;    //thead ativa
    pthread_mutex_t mtx_counters;
    unsigned long dropped;
    unsigned long written;
} tslog_ctx_t;

static tslog_ctx_t g;

// Converte nível numérico em string
static const char* level_str(int lvl) {
    switch (lvl) {
        case TSLOG_DEBUG: return "DEBUG";
        case TSLOG_INFO:  return "INFO";
        case TSLOG_WARN:  return "WARN";
        case TSLOG_ERROR: return "ERROR";
        default: return "LOG";
    }
}

// Função principal da thread escritora: consome da fila e escreve no destino
static void* writer_main(void* arg) {
    (void)arg;
    for (;;) {
        char* line = tsq_pop(g.q);
        if (!line) break; // fila fechada e drenada
        if (g.fp) { fputs(line, g.fp); }
        if (g.to_stderr) { fputs(line, stderr); }
        // Cada linha já termina com '\n'
        free(line);
        pthread_mutex_lock(&g.mtx_counters);
        g.written++;
        pthread_mutex_unlock(&g.mtx_counters);
    }

    if (g.fp) fflush(g.fp);
    if (g.to_stderr) fflush(stderr);
    return NULL;
}

// Inicializa o logger (abre arquivo se necessário, cria fila e thread escritora)
int tslog_init(const char* filepath, int to_stderr) {
    memset(&g, 0, sizeof(g));
    g.level = TSLOG_INFO;
    g.to_stderr = to_stderr ? 1 : 0;
    pthread_mutex_init(&g.mtx_counters, NULL);

    if (filepath) {
        g.fp = fopen(filepath, "a");
        if (!g.fp) {
            g.to_stderr = 1;
        }
    }

    g.q = tsq_create(TSLOG_QUEUE_CAP);
    if (!g.q) {
        if (g.fp) fclose(g.fp);
        pthread_mutex_destroy(&g.mtx_counters);
        return -1;
    }
    g.running = 1;
    if (pthread_create(&g.writer, NULL, writer_main, NULL) != 0) {
        tsq_close(g.q);
        tsq_destroy(g.q);
        if (g.fp) fclose(g.fp);
        pthread_mutex_destroy(&g.mtx_counters);
        return -1;
    }
    return 0;
}
// Define e obtém o nível mínimo de log
void tslog_set_level(int level) { g.level = level; }
int  tslog_get_level(void) { return g.level; }

// Formata uma linha completa de log: timestamp nível [tid=...] [módulo] mensagem\n
static char* fmt_line(int level, const char* module, const char* fmt, va_list ap) {
    char tsbuf[48]; iso8601_now(tsbuf, sizeof(tsbuf));
    unsigned long long tid = tid_as_ull(pthread_self());

    // Formata primeiro a mensagem do usuário
    va_list ap2; va_copy(ap2, ap);
    int needed = vsnprintf(NULL, 0, fmt, ap2);
    va_end(ap2);
    if (needed < 0) return NULL;
    size_t um_len = (size_t)needed + 1;
    char *umsg = (char*)malloc(um_len);
    if (!umsg) return NULL;
    vsnprintf(umsg, um_len, fmt, ap);

    // Monta a linha final
    const char *lvl = level_str(level);
    const char *mod = (module && *module) ? module : "-";

    int final_needed = snprintf(NULL, 0, "%s %s [tid=%llu] [%s] %s\n",
                                tsbuf, lvl, tid, mod, umsg);
    char *line = (char*)malloc((size_t)final_needed + 1);
    if (!line) { free(umsg); return NULL; }
    snprintf(line, (size_t)final_needed + 1, "%s %s [tid=%llu] [%s] %s\n",
             tsbuf, lvl, tid, mod, umsg);
    free(umsg);
    return line;
}

// Função pública de log (não bloqueante: tenta enfileirar com timeout)
void tslog_log(int level, const char* module, const char* fmt, ...) {
    if (level < g.level) return;
    va_list ap; va_start(ap, fmt);
    char* line = fmt_line(level, module, fmt, ap);
    va_end(ap);
    if (!line) return; // se faltar memória, descarta silenciosamente

    if (tsq_push(g.q, line, TSLOG_PUSH_TIMEOUT_MS) != 0) {
        // Não conseguiu enfileirar dentro do timeout: descarta
        free(line);
        pthread_mutex_lock(&g.mtx_counters);
        g.dropped++;
        pthread_mutex_unlock(&g.mtx_counters);
    }
}

void tslog_flush(void) {
    if (g.fp) fflush(g.fp);
    if (g.to_stderr) fflush(stderr);
}


// Encerra o logger: fecha a fila, espera a thread escritora, libera recursos
void tslog_shutdown(void) {
    if (!g.q) return;
    tsq_close(g.q);
    pthread_join(g.writer, NULL);
    tsq_destroy(g.q);
    g.q = NULL;
    if (g.fp) { fclose(g.fp); g.fp = NULL; }
    pthread_mutex_destroy(&g.mtx_counters);
}

// Métricas de descarte e escrita efetiva
unsigned long tslog_get_dropped_count(void) {
    pthread_mutex_lock(&g.mtx_counters);
    unsigned long v = g.dropped;
    pthread_mutex_unlock(&g.mtx_counters);
    return v;
}

unsigned long tslog_get_written_count(void) {
    pthread_mutex_lock(&g.mtx_counters);
    unsigned long v = g.written;
    pthread_mutex_unlock(&g.mtx_counters);
    return v;
}