#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <getopt.h>
#include "tslog.h"

typedef struct {
    int id;
    long nmsgs;
    const char* module;
} worker_arg_t;

static void* worker_main(void* arg) {
    worker_arg_t *wa = (worker_arg_t*)arg;
    for (long i = 0; i < wa->nmsgs; ++i) {
        if ((i % 1000) == 0)
            tslog_debug: ; // label to prevent unused warning if levels are optimized
        tslog_log(TSLOG_DEBUG, wa->module, "worker=%d i=%ld payload=hello-world", wa->id, i);
    }
    tslog_log(TSLOG_INFO, wa->module, "worker %d done (sent %ld msgs)", wa->id, wa->nmsgs);
    return NULL;
}

static void usage(const char* prog) {
    fprintf(stderr,
        "Usage: %s [-t threads] [-n msgs_per_thread] [-o logfile] [--stderr] [--level LEVEL]\n"
        "LEVEL: DEBUG|INFO|WARN|ERROR\n", prog);
}

static int parse_level(const char* s) {
    if (!s) return TSLOG_INFO;
    if (!strcmp(s, "DEBUG")) return TSLOG_DEBUG;
    if (!strcmp(s, "INFO"))  return TSLOG_INFO;
    if (!strcmp(s, "WARN"))  return TSLOG_WARN;
    if (!strcmp(s, "ERROR")) return TSLOG_ERROR;
    return TSLOG_INFO;
}

int main(int argc, char** argv) {
    int nthreads = 8;
    long nmsgs = 100000;
    const char* logfile = "logs/app.log";
    int to_stderr = 0;
    int level = TSLOG_INFO;

    static struct option long_opts[] = {
        {"stderr", no_argument, 0, 1},
        {"level",  required_argument, 0, 2},
        {0,0,0,0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "t:n:o:", long_opts, NULL)) != -1) {
        switch (opt) {
            case 't': nthreads = atoi(optarg); break;
            case 'n': nmsgs = atol(optarg); break;
            case 'o': logfile = optarg; break;
            case 1: to_stderr = 1; break;
            case 2: level = parse_level(optarg); break;
            default: usage(argv[0]); return 1;
        }
    }

    if (tslog_init(logfile, to_stderr) != 0) {
        fprintf(stderr, "tslog_init failed; falling back to stderr only.\n");
        if (tslog_init(NULL, 1) != 0) return 2;
    }
    tslog_set_level(level);

    pthread_t *th = (pthread_t*)calloc((size_t)nthreads, sizeof(pthread_t));
    worker_arg_t *args = (worker_arg_t*)calloc((size_t)nthreads, sizeof(worker_arg_t));

    for (int i = 0; i < nthreads; ++i) {
        args[i].id = i;
        args[i].nmsgs = nmsgs;
        args[i].module = "worker";
        pthread_create(&th[i], NULL, worker_main, &args[i]);
    }

    for (int i = 0; i < nthreads; ++i) pthread_join(th[i], NULL);
    
    tslog_log(TSLOG_INFO, "main", "all workers joined");

    // ====== BEGIN PATCH ======
    // Garanta que a writer drene o que falta e estabilize contadores
    tslog_flush();
    usleep(200000); // 200 ms

    // Leia métricas de descarte antes do shutdown
    unsigned long dropped = tslog_get_dropped_count();

    // Encerre o logger (join na thread escritora e feche arquivo)
    tslog_shutdown();

    // Esperado por nível:
    // - DEBUG: todas as DEBUG + os INFO (1 por thread + 1 do main)
    // - INFO/WARN/ERROR: apenas os INFO (1 por thread + 1 do main)
    long expected = (level <= TSLOG_DEBUG)
        ? (long)nthreads * nmsgs + nthreads + 1
        : (nthreads + 1);

    // Conte linhas atuais do arquivo para validar fisicamente
    long file_lines = 0;
    {
        FILE *fp = fopen(logfile, "r");
        if (fp) {
            int c;
            while ((c = fgetc(fp)) != EOF) if (c == '\n') file_lines++;
            fclose(fp);
        }
    }

    printf("Expected >= %ld lines; dropped=%lu\n", expected, dropped);
    if (file_lines > 0) {
        printf("File line count = %ld (upper bound ~= expected - dropped = %ld)\n",
               file_lines, expected - (long)dropped);
    }

    free(th); free(args);
    return 0;

}