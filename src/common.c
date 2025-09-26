#include "common.h"
#include <stdio.h>
#include <string.h>

/*
 * Gera timestamp no formato ISO-8601 com milissegundos e offset de fuso horário.
 * Exemplo: 2025-09-25T23:11:42.137-03:00
 */
void iso8601_now(char *buf, size_t buflen) {
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
    time_t sec = ts.tv_sec;
    struct tm lt; localtime_r(&sec, &lt);

    // Calcula o offset do fuso horário
    // strftime com %z retorna algo como +HHMM; aqui inserimos ':' para virar +HH:MM
    char tzbuf[8];
    strftime(tzbuf, sizeof(tzbuf), "%z", &lt);
    char tzfmt[7];

    snprintf(tzfmt, sizeof(tzfmt), "%.3s:%.2s", tzbuf, tzbuf+3);


    int ms = (int)(ts.tv_nsec / 1000000);
    snprintf(buf, buflen, "%04d-%02d-%02dT%02d:%02d:%02d.%03d%s",
    lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday,
    lt.tm_hour, lt.tm_min, lt.tm_sec, ms, tzfmt);
}


/*
 * Converte um pthread_t para unsigned long long para impressão portátil.
 * Observação: muitos sistemas permitem cast direto, mas usamos união para
 * evitar warnings dependendo da implementação de pthread_t.
 */
unsigned long long tid_as_ull(pthread_t tid) {
    union { pthread_t t; unsigned long long u; } conv;
    memset(&conv, 0, sizeof(conv));
    conv.t = tid;
    return conv.u;
}