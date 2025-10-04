# LP2 — Etapa 3 (v3-final)

## Tema A — Servidor de Chat Multiusuário (TCP)

Entrega do **sistema completo** com funcionalidades do tema concluídas, **logging integrado** (libtslog), scripts e **relatório final**.

### Como compilar
```bash
make
```

### como executar
```bash
#servidor
./bin/server 9000 logs/server.log

#cliente interativo:
#para uma boa experiencia de ver as trocas de mensagem, crie dois clientes
./bin/client 127.0.0.1 9000 logs/a.log
./bin/client 127.0.0.1 9000 logs/b.log

#comandos do cliente - aqui voce pode trocar mensagens com os clientes
/nick NOME   # define/atualiza seu apelido
/list        # lista usuários conectados
/quit        # encerra a sessão

#clientes robos - voce pode escolher essa opcao
./bin/client 127.0.0.1 9000 --msg "/nick ana" logs/ana.log
./bin/client 127.0.0.1 9000 --msg "olá a todos!" logs/bot.log

#demo automatizada
chmod +x scripts/run_demo.sh
./scripts/run_demo.sh
```


### Logging (libtslog)

Inicialização: tslog_init("logs/arquivo.log", /*to_stderr=*/1)
Log de eventos de rede, mensagens, join/part, erros.
Escrita assíncrona por thread dedicada (da Etapa 1).
Flush no encerramento: tslog_flush().