#!/usr/bin/env python3
import socket
import subprocess
import sys
import time


PASSWORD = "secret"


class IRCClient:
    def __init__(self, port):
        self.sock = socket.create_connection(("127.0.0.1", port), timeout=2)
        self.sock.settimeout(0.2)
        self.buffer = ""

    def send(self, line):
        self.sock.sendall((line + "\r\n").encode())

    def send_raw(self, data):
        self.sock.sendall(data)

    def wait_for(self, text, timeout=2.0):
        deadline = time.time() + timeout
        while time.time() < deadline:
            if text in self.buffer:
                return self.buffer
            try:
                chunk = self.sock.recv(8192)
                if not chunk:
                    break
                self.buffer += chunk.decode(errors="replace")
            except socket.timeout:
                pass
        raise AssertionError("Did not receive %r. Buffer=%r" % (text, self.buffer))

    def clear(self):
        self.buffer = ""
        while True:
            try:
                chunk = self.sock.recv(8192)
                if not chunk:
                    break
            except socket.timeout:
                break

    def close(self):
        try:
            self.sock.close()
        except OSError:
            pass


def free_port():
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    port = s.getsockname()[1]
    s.close()
    return port


def register(client, nick, fragmented=False, batched=False):
    if fragmented:
        client.send_raw(b"PA")
        time.sleep(0.03)
        client.send_raw(("SS %s\r\nNI" % PASSWORD).encode())
        time.sleep(0.03)
        client.send_raw(("CK %s\r\nUSER %s 0 * :%s User\r\n"
                         % (nick, nick, nick)).encode())
    elif batched:
        client.send_raw(
            ("PASS %s\r\nNICK %s\r\nUSER %s 0 * :%s User\r\n"
             % (PASSWORD, nick, nick, nick)).encode()
        )
    else:
        client.send("PASS " + PASSWORD)
        client.send("NICK " + nick)
        client.send("USER %s 0 * :%s User" % (nick, nick))
    client.wait_for(" 001 %s " % nick)
    client.wait_for(" 005 %s " % nick)
    client.wait_for(" 376 %s " % nick)
    client.clear()


def main():
    port = free_port()
    server = subprocess.Popen(
        ["./ircserv", str(port), PASSWORD],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    clients = []
    try:
        time.sleep(0.1)
        if server.poll() is not None:
            out, err = server.communicate(timeout=1)
            raise AssertionError("Server exited early: %s %s" % (out, err))

        a = IRCClient(port); clients.append(a)
        b = IRCClient(port); clients.append(b)
        c = IRCClient(port); clients.append(c)
        d = IRCClient(port); clients.append(d)

        register(a, "alice", fragmented=True)
        register(b, "bob", batched=True)
        register(c, "carol")
        register(d, "dave")

        a.send("ping :hello")
        a.wait_for("PONG ircserv :hello")
        a.clear()

        e = IRCClient(port); clients.append(e)
        e.send("PASS " + PASSWORD)
        e.send("NICK alice")
        e.wait_for(" 433 ")
        e.close()
        clients.remove(e)

        e = IRCClient(port); clients.append(e)
        e.send("PASS wrong")
        e.wait_for(" 464 ")
        e.send("PASS " + PASSWORD)
        e.send("NICK erin")
        e.send("USER erin 0 * :Erin User")
        e.wait_for(" 001 erin ")
        e.close()
        clients.remove(e)

        a.send("JOIN #room")
        a.wait_for("JOIN #room")
        a.wait_for("@alice")
        a.clear()

        b.send("JOIN #room")
        b.wait_for("JOIN #room")
        b.clear()
        a.wait_for("bob!bob@localhost JOIN #room")
        a.clear()

        a.send("PRIVMSG bob :direct hello")
        b.wait_for("PRIVMSG bob :direct hello")
        b.clear()

        a.send("PRIVMSG #room :channel hello")
        b.wait_for("PRIVMSG #room :channel hello")
        b.clear()

        a.send("MODE #room +t")
        a.wait_for("MODE #room +t"); a.clear()
        b.clear()
        b.send("TOPIC #room :blocked")
        b.wait_for(" 482 bob #room ")
        b.clear()

        a.send("MODE #room +o bob")
        b.wait_for("MODE #room +o bob")
        b.clear()
        a.clear()

        b.send("TOPIC #room :allowed topic")
        b.wait_for("TOPIC #room :allowed topic")
        b.clear()
        a.clear()

        a.send("MODE #room +i")
        a.wait_for("MODE #room +i"); a.clear()
        c.send("JOIN #room")
        c.wait_for(" 473 carol #room ")
        c.clear()
        a.send("INVITE carol #room")
        a.wait_for(" 341 alice carol #room")
        a.clear()
        c.wait_for("INVITE carol #room")
        c.clear()
        c.send("JOIN #room")
        c.wait_for("JOIN #room")
        c.clear()
        a.clear(); b.clear()

        a.send("MODE #room -i")
        a.wait_for("MODE #room -i"); a.clear()
        a.send("MODE #room +k roomkey")
        a.wait_for("MODE #room +k roomkey"); a.clear()
        d.send("JOIN #room wrong")
        d.wait_for(" 475 dave #room ")
        d.clear()

        a.send("MODE #room +l 3")
        a.wait_for("MODE #room +l 3"); a.clear()
        d.send("JOIN #room roomkey")
        d.wait_for(" 471 dave #room ")
        d.clear()

        a.send("MODE #room -l")
        a.wait_for("MODE #room -l"); a.clear()
        d.send("JOIN #room roomkey")
        d.wait_for("JOIN #room")
        d.clear()
        a.clear(); b.clear(); c.clear()

        a.send("MODE #room +io bob -i")
        a.wait_for("MODE #room +i")
        a.wait_for("MODE #room +o bob")
        a.wait_for("MODE #room -i")
        a.clear()

        a.send("MODE #room +k-k+k-k one two")
        a.wait_for("MODE #room +k one")
        a.wait_for("MODE #room -k")
        a.wait_for("MODE #room +k two")
        a.clear()
        a.send("MODE #room")
        result = a.wait_for(" 324 alice #room ")
        mode_line = [line for line in result.split("\r\n") if " 324 alice #room " in line][-1]
        if " k" in mode_line or "+k" in mode_line:
            raise AssertionError("Expected key mode removed, got %r" % mode_line)
        a.clear()

        a.send("MODE #room -o bob")
        b.wait_for("MODE #room -o bob")
        b.clear(); a.clear()
        b.send("KICK #room carol :nope")
        b.wait_for(" 482 bob #room ")
        b.clear()

        a.send("KICK #room dave :bye")
        d.wait_for("KICK #room dave :bye")
        d.clear()
        a.clear(); c.clear()

        a.send_raw(b"PING :one\r\nPING :two\r\n")
        a.wait_for("PONG ircserv :one")
        a.wait_for("PONG ircserv :two")
        a.clear()

        a.send("NOTACMD")
        a.wait_for(" 421 alice NOTACMD ")
        a.clear()

        c.send("PART #room :later")
        c.wait_for("PART #room :later")
        c.clear(); a.clear(); b.clear()

        b.send("QUIT :gone")
        a.wait_for("QUIT :gone")
        a.clear()

        c.close()
        clients.remove(c)
        time.sleep(0.1)
        if server.poll() is not None:
            raise AssertionError("Server died after abrupt disconnect")

        stress = []
        for i in range(20):
            cl = IRCClient(port)
            clients.append(cl)
            stress.append(cl)
            register(cl, "u%02d" % i, batched=True)
            cl.send("JOIN #stress")
            cl.wait_for("JOIN #stress")
            cl.clear()
        for cl in stress[:-1]:
            cl.clear()
        stress[0].send("PRIVMSG #stress :stress-message")
        stress[-1].wait_for("PRIVMSG #stress :stress-message")
        if server.poll() is not None:
            raise AssertionError("Server died during stress test")

        print("ALL TESTS PASSED")
        return 0
    finally:
        for client in clients:
            client.close()
        server.terminate()
        try:
            server.wait(timeout=2)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait(timeout=2)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print("TEST FAILED:", exc, file=sys.stderr)
        sys.exit(1)
