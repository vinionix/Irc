# Technical Overview — IRC Server

## Purpose

This project implements an IRC-style server in C++ as part of the 42 curriculum. It focuses on network programming, multiplexed I/O and explicit management of connected clients and channels.

## Entry point

The executable expects a port and server password:

```text
./ircserv <port> <password>
```

`main.cpp` validates the argument count, creates the `Server` object and starts the polling loop.

## Core architecture

```text
TCP socket
   ↓
Server
   ├── poll() set
   ├── connected clients
   ├── channels
   └── command dispatcher
          ↓
      command handlers
```

The server maintains:

- the listening socket;
- a map of connected clients by file descriptor;
- a collection of channels;
- a vector of `pollfd` structures;
- server password and port state.

## Network model

The implementation uses POSIX sockets and `poll()` instead of creating one thread per client. This is an event-driven approach: the process waits for activity across multiple file descriptors and then handles the descriptors that became ready.

Relevant APIs and concepts include:

- `socket`;
- TCP/IP;
- `poll`;
- non-blocking file descriptors;
- `sockaddr_in`;
- connection lifecycle;
- buffering partial client input;
- protocol parsing.

## Command architecture

Commands are parsed into a `ParsedCommand` structure containing the command name and its arguments. The `Server` exposes dedicated handlers for commands including:

- PASS
- NICK
- USER
- JOIN
- PART
- MODE
- TOPIC
- KICK
- INVITE
- PRIVMSG
- QUIT

A dispatcher maps parsed commands to handler methods.

## Domain objects

The source tree separates server, client and channel responsibilities:

```text
src/
├── server/
├── client/
└── channel/
```

This keeps protocol state from collapsing into a single class and makes client/channel behavior easier to reason about.

## Important engineering challenges

- handling multiple clients without blocking the process;
- preserving incomplete input until a full protocol line arrives;
- registering clients only after required handshake state is valid;
- validating nicknames and avoiding duplicates;
- broadcasting messages to channel members;
- keeping channel membership and operator state consistent;
- disconnecting clients without leaving stale descriptors or references.

## Testing ideas

Useful validation scenarios include:

- connect multiple clients concurrently;
- attempt authentication with a wrong password;
- attempt duplicate nicknames;
- join and leave channels;
- send private messages;
- test channel operator commands;
- disconnect abruptly;
- send multiple commands in one TCP packet;
- split one command across multiple packets.

## Portfolio value

This repository demonstrates lower-level networking skills that are directly useful in backend, infrastructure and distributed systems work: socket programming, stateful protocols, event loops, buffering, client lifecycle and fault handling.
