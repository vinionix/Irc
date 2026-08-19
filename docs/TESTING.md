# Testing Guide

## Automated integration suite

Run:

```bash
make test
```

The suite starts `ircserv` on a free local port and uses several TCP clients to exercise
the server.

Covered scenarios:

- fragmented registration data across multiple TCP writes;
- multiple IRC commands in a single TCP write;
- successful registration and initial numeric replies;
- incorrect connection password;
- duplicate nickname rejection;
- command name normalization;
- `PING` / `PONG`;
- channel creation and first-user operator assignment;
- direct `PRIVMSG`;
- channel `PRIVMSG`;
- `TOPIC` restrictions;
- `MODE +i/-i`;
- `MODE +t/-t`;
- `MODE +k/-k`;
- `MODE +o/-o`;
- `MODE +l/-l`;
- combined mode strings and additional mode tokens;
- repeated mode transitions such as `+k-k+k-k`;
- invite-only channel behavior;
- `INVITE`;
- channel key validation;
- channel user limit validation;
- operator permission failures;
- `KICK`;
- `PART`;
- `QUIT` propagation;
- unknown command errors;
- abrupt peer disconnection;
- a basic 20-client stress scenario.

## Subject-style partial packet test

A manual equivalent of the subject's low-bandwidth test can be performed with `nc`.

Start the server:

```bash
./ircserv 6667 secret
```

Connect:

```bash
nc -C 127.0.0.1 6667
```

Register, then send a command in pieces. For example, type `PI`, send it without closing
the connection, then send `NG :split-test`. The server must wait for a complete IRC line
before dispatching it and should eventually answer with a `PONG`.

## Static I/O checks

The mandatory design should preserve these invariants:

- one `poll()` call in the event loop;
- `recv()` only after `POLLIN`;
- `send()` only after `POLLOUT`;
- `accept()` only after the listening socket reports `POLLIN`;
- listening and accepted sockets use `O_NONBLOCK`;
- no post-`send()`/`recv()` retry logic based on `errno`.

## HexChat reference-client pass

Reference client: **HexChat**.

The current environment used for the automated suite does not include HexChat, so this
part is intentionally a manual final check rather than a claimed automated result.

Before evaluation, verify:

1. Start `ircserv`.
2. Configure HexChat with `127.0.0.1`, the selected port, and server password.
3. Confirm PASS/NICK/USER registration without connection errors.
4. Join a channel from two HexChat clients.
5. Exchange a direct message.
6. Exchange a channel message.
7. Verify TOPIC view/change behavior.
8. Verify `+i`, `+t`, `+k`, `+o`, and `+l`.
9. Verify KICK and INVITE as a channel operator.
10. Confirm a regular user receives the expected permission error for restricted actions.
11. Disconnect one client abruptly and confirm the other client and server remain usable.

Earlier versions of the project were already exercised with HexChat for connection,
JOIN, and numeric error behavior. The final pass above is still recommended after the
latest MODE and output-buffer changes.
