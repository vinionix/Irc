# IRC Server — C++ / 42 Rio

Servidor IRC desenvolvido em C++ com sockets TCP e multiplexação de I/O usando `poll()`.

O projeto faz parte da formação da 42 e trabalha conceitos de redes e sistemas que normalmente ficam escondidos por frameworks de alto nível: sockets, file descriptors, buffers, protocolo textual, estado de clientes, canais e tratamento de múltiplas conexões.

## Objetivo

Construir um servidor capaz de receber múltiplos clientes simultaneamente, interpretar comandos no estilo IRC e manter o estado necessário para usuários e canais sem criar uma thread por conexão.

## Tecnologias e conceitos

- C++
- POSIX sockets
- TCP/IP
- `poll()`
- file descriptors
- parsing de protocolo textual
- buffers de entrada
- gerenciamento de clientes e canais
- comandos e respostas numéricas do IRC
- Makefile

## Arquitetura

```text
Cliente TCP ─┐
Cliente TCP ─┼─> poll() ─> Server ─> parser ─> dispatcher ─> handlers
Cliente TCP ─┘                  │
                               ├── Client state
                               └── Channel state
```

O servidor mantém o socket de escuta, os clientes conectados, os canais existentes e o conjunto de file descriptors observado pelo `poll()`.

A árvore principal é separada por domínio:

```text
src/
├── server/
│   ├── Server.cpp
│   ├── Server.hpp
│   └── replies.hpp
├── client/
└── channel/
```

## Comandos identificados na implementação

A interface do `Server` possui handlers dedicados para:

- `PASS`
- `NICK`
- `USER`
- `JOIN`
- `PART`
- `MODE`
- `TOPIC`
- `KICK`
- `INVITE`
- `PRIVMSG`
- `QUIT`

Os comandos são convertidos para uma estrutura com nome + argumentos antes do despacho para o handler apropriado.

## Como compilar

```bash
git clone https://github.com/vinionix/Irc.git
cd Irc
make
```

## Como executar

O programa recebe porta e senha do servidor:

```bash
./ircserv <port> <password>
```

Exemplo:

```bash
./ircserv 6667 minha_senha
```

O nome exato do executável pode ser confirmado no `Makefile` caso seja alterado durante o desenvolvimento.

## O que torna esse projeto interessante

A parte difícil não é apenas abrir um socket. O servidor precisa manter consistência enquanto vários clientes enviam dados em momentos diferentes.

Entre os desafios estão:

- lidar com múltiplos file descriptors sem bloquear o processo;
- acumular dados quando uma mensagem TCP chega fragmentada;
- processar múltiplos comandos recebidos de uma vez;
- manter nicknames únicos;
- controlar registro/autenticação do cliente;
- manter membros e operadores de canais;
- remover corretamente um cliente desconectado;
- distribuir mensagens sem corromper o estado do servidor.

## Testes úteis

Para avaliar o servidor de forma mais realista, vale testar:

1. vários clientes conectados ao mesmo tempo;
2. senha incorreta;
3. nickname duplicado;
4. criação e entrada em canais;
5. mensagens privadas;
6. comandos de operador;
7. desconexão abrupta;
8. comando dividido em mais de um pacote TCP;
9. vários comandos enviados no mesmo pacote.

## Status

Projeto em desenvolvimento/validação. A estrutura de servidor, cliente, canal, parsing e handlers de comandos está presente no repositório.

## O que este projeto demonstra

- programação de redes em baixo nível;
- entendimento prático de TCP e sockets;
- multiplexação com `poll()`;
- modelagem de estado em um protocolo cliente-servidor;
- C++ aplicado a sistemas;
- troubleshooting de concorrência de I/O sem depender de frameworks de rede.

## Documentação

- [Technical Overview](docs/TECHNICAL_OVERVIEW.md) — arquitetura, modelo de rede, responsabilidades e cenários de teste.

## Autor

Desenvolvido por [Vinícius Fidelis](https://github.com/vinionix) durante a formação na 42 Rio.
