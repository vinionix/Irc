# Technical Overview — ft_irc

## Purpose

This project implements an IRC server in C++98 with POSIX TCP sockets and a single
`poll()`-driven event loop. The implementation focuses on the mandatory `ft_irc`
features: registration, channels, private/channel messaging, operators, and the required
channel operator commands.

## Entry point

```text
./ircserv <port> <password>
```

`main.cpp` validates the argument count, constructs `Server`, and starts the event loop.

## Architecture

```text
TCP clients
    |
    v
 poll()
    |
    v
 Server
 ├── listening socket
 ├── pollfd vector
 ├── map<int, Client>
 ├── vector<Channel>
 ├── input parsing / command dispatch
 └── output queue management
       |
       +--> Commands.cpp
       +--> Mode.cpp
```

### Server

`Server` owns the listening socket and the connected socket lifecycle. It also coordinates
clients, channels, parsing, command dispatch, queued replies, and disconnections.

### Client

A `Client` represents one TCP connection and stores:

- file descriptor;
- nickname;
- USER registration data;
- password/registration state;
- joined channel names;
- persistent input buffer;
- persistent output buffer.

### Channel

A `Channel` stores:

- name;
- topic;
- key/password;
- invite-only state;
- topic-restricted state;
- user limit;
- member file descriptors;
- operator file descriptors;
- invited file descriptors.

## Non-blocking network model

The implementation uses one `poll()` call to wait for all network readiness.

- Listener `POLLIN` -> `accept()` one client.
- Client `POLLIN` -> `recv()` one chunk.
- Client `POLLOUT` -> `send()` queued output.
- Error/hangup events -> connection cleanup.

Both the listening socket and accepted sockets are set with:

```text
fcntl(fd, F_SETFL, O_NONBLOCK)
```

Command handlers do not directly call `send()`. They append IRC lines to the target
client's output buffer. The poll descriptor is then interested in `POLLOUT`. If a
non-blocking `send()` writes only part of the buffer, only the written prefix is erased;
the remainder stays queued for the next writable event.

This design also avoids retry decisions based on `errno` after `recv()` or `send()`.

## TCP input buffering

TCP provides a byte stream rather than IRC message boundaries. Each `Client` therefore
keeps an `inBuffer`.

Received bytes are appended to the buffer. The server extracts and dispatches only
complete `\r\n`-terminated lines. A partial command remains buffered until more bytes
arrive, while several complete commands received together are processed one by one.

## Parser and dispatcher

`parseCommand()` produces:

```text
ParsedCommand
├── command
└── args[]
```

Command names are normalized to uppercase. A trailing IRC parameter introduced by `:`
is kept as one argument, including spaces.

A table maps supported command names to `Server` member-function handlers.

Supported handlers include:

- PASS
- NICK
- USER
- JOIN
- PART
- PRIVMSG
- QUIT
- PING / PONG
- minimal CAP compatibility
- KICK
- INVITE
- TOPIC
- MODE

## Registration

A newly accepted socket is connected but not registered. Registration is completed only
when the client has:

1. supplied the correct connection password;
2. selected a valid, unused nickname;
3. supplied USER information.

After registration, the server sends the initial numeric replies including welcome and
basic server capability information.

## Channel lifecycle

If a client joins a channel that does not yet exist, the channel is created and that
client becomes the initial operator.

JOIN enforces:

- invite-only mode (`+i`);
- channel key (`+k`);
- user limit (`+l`).

Clients and channels maintain matching membership state. PART, KICK, QUIT, and abrupt
socket closure remove membership consistently. Empty channels are removed.

## Messaging

`PRIVMSG` supports:

- nickname targets for direct messages;
- channel targets for messages broadcast to every other channel member.

Channel broadcasts use client file descriptors to resolve the current `Client` objects.

## Operator commands

### KICK

Channel operators can remove a member from a channel. The KICK message is broadcast
before membership is removed.

### INVITE

Invited client file descriptors are stored by the channel and consumed by JOIN when the
channel is invite-only.

### TOPIC

TOPIC can be queried by a member. When `+t` is active, topic changes require channel
operator privileges.

### MODE

Mandatory modes:

- `i` — invite-only;
- `t` — operator-only topic changes;
- `k` — channel key;
- `o` — operator privilege;
- `l` — user limit.

The mode parser walks mode strings from left to right while retaining the latest `+` or
`-` sign. Modes consume following command arguments only when required. It accepts both
combined changes and additional signed mode tokens in one command.

## Disconnect handling

Disconnect cleanup:

1. gathers other users sharing channels with the leaving client;
2. sends one deduplicated QUIT notification per recipient;
3. removes the client from channel member/operator/invite state;
4. removes its `pollfd`;
5. closes the socket;
6. removes empty channels;
7. later erases the disconnected `Client` object from the map.

`SIGPIPE` is ignored so a write to a peer that disappeared cannot terminate the whole
server process.

## Validation

The repository contains `tests/irc_test.py`, runnable with:

```bash
make test
```

It validates fragmented and batched TCP input, registration, messaging, channel state,
all mandatory modes, operator restrictions, PART/QUIT/KICK/INVITE, abrupt disconnects,
and a basic multi-client stress scenario.

See `TESTING.md` for the automated coverage and final HexChat reference-client checklist.
