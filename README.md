# LP2 — Etapa 1 (v1-logging)

## Tema A — Servidor de Chat Multiusuário (TCP)

Esta entrega implementa a **libtslog** (biblioteca de logging thread-safe) e o teste de carga **log_stress**.

## Como compilar
```bash
make
```

## Como executar (exemplos)
```bash
rm -f logs/app.log
# 8 threads, 200k msgs cada, nível DEBUG, grava em logs/app.log e stderr
./bin/log_stress -t 16 -n 200000 --stderr --level DEBUG -o logs/app.log


# Somente INFO em arquivo
rm -f logs/app.log
./bin/log_stress -t 8 -n 50000 -o logs/app.log --level INFO
```


## Estrutura de diretórios
```
      /project-root
        ├─ src/
        │   ├─ tslog.c/.h
        │   ├─ threadsafe_queue.c/.h
        │   ├─ common.c/.h
        │   ├─ server.h  (stub Tema A)
        │   └─ client.h  (stub Tema A)
        ├─ tests/
        │   └─ log_stress.c
        ├─ logs/ (saída)
        ├─ Makefile
        └─ README.md
```

## Critérios
- Chamadas `tslog_log` são **não-bloqueantes** (enfileiram); se a fila lotar por muito tempo, há **descarte com contagem**.
- Escrita feita por **uma thread dedicada**, garantindo linhas **íntegras** e sem interleaving.
- **POSIX (pthreads)**, sem dependências externas.

## Próximos passos (Etapa 2+)
- Implementar o servidor TCP do **Tema A (Chat)** usando a `libtslog`.
- Pool de conexões, broadcast, comandos básicos (lista/quit), shutdown limpo.