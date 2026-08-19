# ft_irc Defense Guide

## 1. Build

```bash
make fclean
make
```

Explain:

- the target is `ircserv`;
- compilation uses `c++`;
- flags are `-Wall -Wextra -Werror -std=c++98`;
- a second `make` should not relink unnecessarily.

## 2. Start the server

```bash
./ircserv 6667 secret
```

Explain that the two arguments are the listening TCP port and the connection password.

## 3. Network initialization

Walk through:

1. `socket(AF_INET, SOCK_STREAM, 0)`
2. `setsockopt(... SO_REUSEADDR ...)`
3. `bind`
4. `fcntl(fd, F_SETFL, O_NONBLOCK)`
5. `listen`
6. register the listening fd in the poll vector

Be ready to explain the difference between the listening socket and accepted client
sockets.

## 4. The poll loop

The central rule is that the project uses one event loop for all network I/O.

Explain:

- `POLLIN` on the listening socket allows `accept`;
- every accepted client is also set to non-blocking mode;
- `POLLIN` on a client allows one `recv`;
- command handlers queue output instead of calling `send`;
- queued output enables `POLLOUT`;
- `POLLOUT` allows one `send`;
- partial writes remain in the client's output buffer.

Important subject point: the implementation does not call `recv` or `send` merely
because the fd is non-blocking. Read/write operations are gated by `poll()` readiness.

## 5. Partial TCP input

Show that TCP does not preserve application message boundaries.

A client stores received bytes in `inBuffer`. The server only extracts a command when it
finds `\r\n`. If only half a command arrived, the bytes remain there until the next
`recv`.

Demonstrate with `nc` or `make test`.

## 6. Parsing and dispatch

A parsed command contains:

```text
command
args[]
```

Explain the trailing parameter rule:

```text
PRIVMSG #room :hello everyone
```

becomes a command plus two arguments, where `hello everyone` is one final argument.

The dispatcher maps command names to member-function handlers.

## 7. Registration

Demonstrate:

```text
PASS secret
NICK alice
USER alice 0 * :Alice Example
```

The client is only marked registered when password, nickname, and username are all
present.

Explain the difference between:

- connected TCP client;
- authenticated client;
- registered IRC client.

## 8. Channels and messaging

Demonstrate with two clients:

```text
JOIN #room
PRIVMSG #room :hello
PRIVMSG bob :private hello
PART #room :bye
```

The first user to create a channel becomes its initial operator.

Channel messages are forwarded to every other member of that channel.

## 9. Operator commands

### KICK

```text
KICK #room bob :reason
```

Only a channel operator can remove another channel member.

### INVITE

```text
INVITE bob #room
```

An invite is stored in the channel and checked by JOIN when `+i` is active.

### TOPIC

```text
TOPIC #room
TOPIC #room :new topic
```

When `+t` is active, only operators can change it.

### MODE

Mandatory modes:

```text
+i / -i
+t / -t
+k key / -k
+o nick / -o nick
+l number / -l
```

The parser walks mode strings from left to right while carrying the current sign.

Example:

```text
MODE #room +io bob -i
```

means:

```text
+i
+o bob
-i
```

Example:

```text
MODE #room +kol secret bob 10
```

consumes arguments in this order:

```text
+k -> secret
+o -> bob
+l -> 10
```

## 10. Disconnect lifecycle

On QUIT or socket closure:

- notify peers that share channels;
- remove the fd from channel member/operator/invite sets;
- remove the poll descriptor;
- close the socket;
- remove empty channels;
- remove the disconnected Client object safely.

QUIT recipients are deduplicated so users sharing multiple channels do not receive the
same QUIT several times.

## 11. Tests to demonstrate

```bash
make test
```

Point out coverage for:

- packet fragmentation;
- batched commands;
- authentication;
- duplicate nicks;
- direct/channel messages;
- all mandatory modes;
- KICK/INVITE/TOPIC;
- abrupt disconnect;
- multiple clients.

Also perform the final HexChat checklist in `docs/TESTING.md`.

## Likely questions

### Why `poll()`?

It lets one process wait for readiness on many file descriptors instead of blocking on a
single socket or creating one process/thread per client.

### Why does each client need an input buffer?

Because TCP is a byte stream. One IRC command can arrive in several packets, and several
commands can arrive in one packet.

### Why does each client need an output buffer?

A non-blocking `send()` may write only part of the requested bytes. The unsent remainder
must survive until the fd becomes writable again.

### What does `SO_REUSEADDR` do?

It allows useful address reuse cases such as rebinding after previous connections; it
does not allow two active listening sockets to own the same address/port combination in
the normal case.

### Why set accepted sockets to non-blocking too?

The listening socket and each connected client fd are different descriptors. Accepted
client sockets must be explicitly configured so a client operation cannot stall the
event loop.

### What is the difference between NICK and USER?

`NICK` is the public IRC nickname used as a message target. `USER` supplies registration
information such as username and real name.

### Why is the first channel member an operator?

A newly created channel needs an initial channel operator so operator-only actions are
possible.

### How does MODE know which parameter belongs to which mode?

Only parameterized modes consume the next argument. `i` and `t` do not. `+k`, `+o`,
`-o`, and `+l` consume arguments, while `-k` and `-l` do not in this implementation.

### Why ignore SIGPIPE?

Writing to a connection that the peer has already closed can otherwise terminate the
process on systems where `send()` raises SIGPIPE. Ignoring it keeps disconnect errors
inside the server's normal I/O handling instead of crashing the process.
