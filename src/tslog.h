#ifndef TSLOG_H
#define TSLOG_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif


// Níveis de log
enum tslog_level {
    TSLOG_DEBUG = 10,
    TSLOG_INFO  = 20,
    TSLOG_WARN  = 30,
    TSLOG_ERROR = 40
};

// Inicializa o logger.
// Se 'filepath' for NULL, apenas 'stderr' é usado quando 'to_stderr' != 0.
// Retorna 0 em caso de sucesso e -1 em caso de falha.
int  tslog_init(const char* filepath, int to_stderr);
void tslog_set_level(int level);
int  tslog_get_level(void);

// Loga uma mensagem (formato printf
void tslog_log(int level, const char* module, const char* fmt, ...);

// Força a escrita de qualquer dado pendente
void tslog_flush(void);

// Encerra o logger, aguardando a thread de escrita terminar e fechando o arquivo
void tslog_shutdown(void);

// Retorna contadores de métricas
unsigned long tslog_get_dropped_count(void);
unsigned long tslog_get_written_count(void);

#ifdef __cplusplus
}
#endif

#endif 
