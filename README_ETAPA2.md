LP2 — Etapa 2 (v2-cli)
Tema A — Servidor de Chat Multiusuário (TCP)

Esta entrega implementa um protótipo CLI de comunicação cliente/servidor em rede (chat simples com broadcast) integrado à libtslog da Etapa 1 (logger thread-safe com fila e writer dedicado). Inclui scripts de teste para simular múltiplos clientes e a marcação de tag v2-cli.

## Como compilar
```bash
make
```

## Como executar (exemplos)
```bash
# 1) Servidor (porta 9000; logs em logs/server.log)
./scripts/run_server.sh 9000

# 2) Cliente interativo (conecta ao servidor e permite digitar mensagens)
./bin/client 127.0.0.1 9000 logs/client.log
./bin/client 127.0.0.1 9000 --msg "oi do bot" logs/client-bot.log

# 3) Simulação de carga: N clientes “robôs”, cada um envia 1 mensagem e encerra
./scripts/run_clients.sh 127.0.0.1 9000 10
```

## estrutura de diretorios

/project-root
  ├─ src/
  │   ├─ tslog.c/.h              # (da Etapa 1 — logger thread-safe)
  │   ├─ server.c                # servidor TCP (broadcast) usando tslog
  │   └─ client.c                # cliente CLI (interativo e --msg) usando tslog
  ├─ scripts/
  │   ├─ run_server.sh           # sobe o servidor
  │   └─ run_clients.sh          # dispara N clientes “robôs”
  ├─ logs/                       # saída de logs
  ├─ bin/                        # binários compilados
  ├─ Makefile
  └─ README-ETAPA2.md



## Uso do logger (libtslog)

O servidor e o cliente usam a mesma libtslog da Etapa 1:
- inicialização do logger (arquivo + opção de também logar em stderr);
- níveis (DEBUG, INFO, WARN, ERROR);
- escrita feita por thread dedicada, sem interleaving e com fila;
- descarte controlado caso a fila sature (mantendo contagem).

### Exemplos de mensagens registradas:

Servidor: início/porta, conexões e desconexões, mensagens recebidas, erros de socket.
Cliente: conexão/encerramento, envio/recebimento e eventos de erro.

## Funcionalidades

Servidor (TCP): aceita múltiplos clientes (via select()), recebe mensagens e retransmite para todos os demais (broadcast).

Cliente (CLI):
- Interativo: teclado → socket; socket → tela.
- Scriptado: --msg "texto" para enviar uma mensagem e encerrar (usado pelos scripts de teste).

Scripts:

- run_server.sh: sobe o servidor e direciona logs para logs/server.log.
- run_clients.sh: simula N clientes “robôs”, cada um enviando uma mensagem (gera logs/client-<i>.log).

## Critérios atendidos

Cliente/servidor mínimo em rede (chat/broadcast) funcionando.
Logging integrado com a libtslog da Etapa 1 (fila + writer dedicado).
Scripts de teste para simular conexões concorrentes.
Makefile para build padronizado (make).
Tag da entrega: v2-cli.