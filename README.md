*This project has been created as part of the 42 curriculum by vfidelis.*

# ft_irc

## Description

`ft_irc` is an IRC server written in C++98. The project implements a small but functional
subset of the IRC protocol and focuses on low-level TCP networking, non-blocking I/O,
protocol parsing, client state, channel state, and event-driven server design.

The executable accepts a listening port and a connection password:

```text
./ircserv <port> <password>
```

A client authenticates with `PASS`, chooses a nickname with `NICK`, sends its user
information with `USER`, and can then join channels and exchange messages.

The server supports multiple simultaneous clients using a single `poll()` loop. Reads,
writes, accepts, and the listening socket are all coordinated through that event loop.
Outgoing IRC replies are queued per client and are only written after `poll()` reports
`POLLOUT`, so partial writes can be preserved without blocking the server.

### Supported commands

- `PASS`
- `NICK`
- `USER`
- `JOIN`
- `PART`
- `PRIVMSG`
- `QUIT`
- `PING`
- `PONG`
- `CAP` (minimal compatibility handling)
- `KICK`
- `INVITE`
- `TOPIC`
- `MODE`

The mandatory channel modes are implemented:

- `i` — set/remove invite-only mode
- `t` — restrict/unrestrict `TOPIC` changes to channel operators
- `k` — set/remove the channel key
- `o` — give/take channel operator privileges
- `l` — set/remove the channel user limit

`MODE` strings are processed from left to right. The active `+` or `-` operation is
preserved until another sign appears, and parameters are consumed only by modes that
need them. This supports commands such as:

```text
MODE #room +it
MODE #room +kol secret bob 10
MODE #room +io bob -i
MODE #room +k-k+k-k one two
```

### Implementation strategy

The project is separated into three main domain objects:

```text
Server
├── listening socket and poll descriptors
├── connected Client objects
├── Channel collection
├── IRC parser and dispatcher
└── command handlers

Client
├── socket file descriptor
├── registration state
├── nickname and USER data
├── joined channels
├── input buffer
└── output buffer

Channel
├── name and topic
├── members
├── operators
├── invited users
├── channel key
├── invite-only/topic-restricted flags
└── user limit
```

TCP is a byte stream, so a single `recv()` is not assumed to contain one complete IRC
command. Every client has a persistent input buffer. Complete `\r\n` terminated lines
are extracted and parsed while incomplete data remains buffered for the next read.

The parser separates the command name from parameters and preserves an IRC trailing
parameter beginning with `:` as one argument, including spaces.

For outgoing data, handlers never call `send()` directly. They append protocol lines to
the target client's output buffer and enable `POLLOUT` for that descriptor. The event
loop performs at most one `send()` for a writable event and removes only the bytes that
were actually transmitted.

The first client that creates a channel becomes its operator. Channel membership,
operator membership, invitations, keys, limits, and topic restrictions are then enforced
by the command handlers.

## Instructions

### Requirements

- A C++98-compatible compiler
- POSIX sockets
- `poll()`
- `make`
- Python 3 only if you want to run the optional automated test suite

### Compilation

```bash
make
```

The project is compiled with:

```text
-Wall -Wextra -Werror -std=c++98
```

To rebuild from scratch:

```bash
make re
```

To remove object files:

```bash
make clean
```

To remove object files and the executable:

```bash
make fclean
```

### Execution

```bash
./ircserv <port> <password>
```

Example:

```bash
./ircserv 6667 secret
```

### Testing with netcat

Connect:

```bash
nc -C 127.0.0.1 6667
```

Register:

```text
PASS secret
NICK alice
USER alice 0 * :Alice Example
```

Then try:

```text
JOIN #general
PRIVMSG #general :hello
PING :test
MODE #general
```

The repository also contains an automated integration suite:

```bash
make test
```

It covers registration, fragmented input, several commands in one TCP packet, direct
messages, channel messages, channel modes, operator permissions, invites, keys, limits,
KICK, PART, QUIT, abrupt disconnections, and a basic multi-client stress scenario.

### Reference client

The selected reference IRC client is **HexChat**. Earlier project milestones were
validated with HexChat. Because the latest changes include the non-blocking output path
and the complete `MODE` implementation, a final manual HexChat pass should still be run
before evaluation. The expected scenarios are listed in `docs/TESTING.md`.

## Resources

Classic references used for the project:

- RFC 2812 — Internet Relay Chat: Client Protocol
- RFC 1459 — Internet Relay Chat Protocol
- Linux/POSIX manual pages for `socket`, `bind`, `listen`, `accept`, `recv`, `send`,
  `fcntl`, and `poll`
- 42 `ft_irc` subject, version 11.0

### AI usage

AI tools were used as a development assistant for specific parts of the project:

- explaining IRC protocol concepts and command syntax;
- reviewing the architecture of `Server`, `Client`, and `Channel`;
- helping design and implement the mandatory `MODE` handling;
- reviewing the network loop against the subject's non-blocking `poll()` requirements;
- refactoring outgoing messages to use per-client output buffers and `POLLOUT`;
- drafting and expanding automated integration tests;
- reviewing edge cases such as partial TCP input, repeated mode changes, disconnects,
  and operator permissions;
- drafting and reviewing project documentation.

AI-generated suggestions were compiled and exercised with integration tests before being
kept in the repository. The author is responsible for reviewing, understanding, and
being able to defend every retained change. Peer review remains an important final
quality check, as required by the subject.

## Documentation

- [`docs/TECHNICAL_OVERVIEW.md`](docs/TECHNICAL_OVERVIEW.md) — architecture overview
- [`docs/TESTING.md`](docs/TESTING.md) — automated and manual validation plan
- [`docs/DEFENSE_GUIDE.md`](docs/DEFENSE_GUIDE.md) — evaluation walkthrough and likely questions

## Bonus

No bonus feature is implemented. File transfer and bots are intentionally left out so
the mandatory part can remain the priority.
