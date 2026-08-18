*This project has been created as part of the 42 curriculum by gada-sil, vfidelis, yfaustin.*

# ft_irc

## Description

`ft_irc` is an IRC server written from scratch in C++98. The goal of the project
is to implement, without relying on any third-party networking or IRC library,
the core of the IRC protocol needed to authenticate clients, exchange messages
and manage channels — while handling any number of clients concurrently through
a single, non-blocking event loop.

The server is single-threaded and multiplexes every client socket with
[`poll()`](https://man7.org/linux/man-pages/man2/poll.2.html), so no read or
write ever blocks the whole process. A real IRC client (HexChat, irssi,
WeeChat...) or a simple TCP tool such as `nc` can connect to it, authenticate
and start chatting on channels.

Currently implemented commands:

| Command   | Description                                                        |
|-----------|---------------------------------------------------------------------|
| `PASS`    | Authenticate against the server password                            |
| `NICK`    | Set or change the client's nickname                                  |
| `USER`    | Register the client's username/realname and complete registration    |
| `JOIN`    | Join (or create) a channel, honouring key (`+k`) and limit (`+l`)    |
| `PART`    | Leave a channel                                                      |
| `TOPIC`   | View or change a channel's topic (respects `+t` when set)            |
| `KICK`    | Remove a client from a channel (operator only)                       |
| `INVITE`  | Invite a client to a channel (operator only when `+i` is set)        |
| `PRIVMSG` | Send a message to a client or to a channel                           |
| `QUIT`    | Disconnect from the server with an optional quit message             |

The first client to `JOIN` a channel is automatically promoted to channel
operator. `MODE` is scaffolded in the dispatcher but not implemented yet, so
channel modes can currently only be set implicitly (e.g. a key passed to
`JOIN` when a channel is created).

## Instructions

### Requirements

- A C++98-capable compiler (`c++`/`g++`/`clang++`)
- `make`
- A POSIX/Unix-like system (Linux or macOS)

### Compilation

```sh
make        # builds the `ircserv` binary
make clean  # removes object files
make fclean # removes object files and the binary
make re     # fclean + all
```

### Running the server

```sh
./ircserv <port> <password>
```

- `port` — the TCP port the server listens on
- `password` — the password clients must provide via `PASS` before registering

### Connecting

With an IRC client, connect to `127.0.0.1:<port>` and log in with the server
password. It can also be tested directly over a raw TCP connection:

```sh
nc -C 127.0.0.1 <port>
PASS <password>
NICK alice
USER alice 0 * :Alice
JOIN #general
```

## Resources

- [RFC 1459 — Internet Relay Chat Protocol](https://datatracker.ietf.org/doc/html/rfc1459)
- [RFC 2812 — IRC: Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812)
- [`poll(2)` man page](https://man7.org/linux/man-pages/man2/poll.2.html)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [modern IRC documentation (modern.ircdocs.horse)](https://modern.ircdocs.horse/)

**AI usage:** Claude (Anthropic) was used as a documentation assistant to
explore this codebase and draft this `README.md` — summarizing the server
architecture, the set of commands actually implemented in
[`Server.cpp`](src/server/Server.cpp), and the build/run instructions derived
from the [`Makefile`](Makefile). No AI-generated code was merged into the
server implementation itself through this task.
