# Concurrent HTTP-Style Web Server ⚙️

**A multi-process file server in C, modeled on HTTP GET semantics — parses GET requests, serves files over raw sockets, and forks a child process per connection to handle simultaneous clients. Verified under load with zero memory leaks.**

![C](https://img.shields.io/badge/C-00599C?logo=c&logoColor=white)
![Platform](https://img.shields.io/badge/platform-Unix%2FLinux-lightgrey)

---

## Overview

This project extends a basic echo server into a **concurrent GET-request server** built directly on the Unix sockets API. It accepts client connections, parses `GET <filename>` requests, and serves files from disk — forking a dedicated child process for each connection so multiple clients are served at once. Careful process reaping and Valgrind-verified memory management keep it leak-free under load.

**Note on protocol:** this speaks a simplified request format modeled on HTTP GET (`GET <filename>`), not full HTTP/1.x — there's no status line or headers in the response, so it isn't compatible with `curl` or a browser. It's tested with the included `client` program. Framing it this way up front beats a recruiter trying `curl` and hitting a wall.

## ✨ Features

- **GET request parsing** — parses request lines and serves the requested file over sockets
- **Process-per-connection concurrency** — `fork()` a child per client; verified handling **1,500+ simultaneous connections** with 100% success in load testing
- **Robust child reaping** — `SIGCHLD` handler + `waitpid()` prevent zombie processes (confirmed: zero leftover processes after 5,000+ sequential requests)
- **Memory-safe** — verified with Valgrind across 30 concurrently-forked child processes under load: **0 leaks, 0 errors** in every process
- **Graceful error handling** — proper error responses for missing files / bad requests

## 🏗️ How it works

```
              ┌──────────────┐
  client ───▶ │  accept()    │
              └──────┬───────┘
                     │ fork()
        ┌────────────┴────────────┐
        ▼                         ▼
   child process             parent keeps
   parse GET, serve file      accept()-ing
   exit()                     SIGCHLD ─▶ waitpid() reaps child
```

## 🛠️ Tech / Concepts

- **Language:** C
- **APIs:** POSIX sockets (`socket`, `bind`, `listen`, `accept`)
- **Concurrency:** `fork()`, process-per-connection model
- **Signals:** `SIGCHLD` + `waitpid()` for child reaping
- **Tooling:** Make, Valgrind, Unix shell

## 📊 Performance (measured)

- **1,500+** simultaneous connections handled, 100% success rate
- **~3,850 requests/sec** sustained at 200-way concurrency (5,000/5,000 requests succeeded)
- **Zero** zombie processes after 5,000+ requests — SIGCHLD reaping confirmed under load
- **Zero** memory leaks or errors (Valgrind `--leak-check=full --trace-children=yes`, across 30 concurrent child processes)

## 🚀 Build & Run

```bash
git clone https://github.com/aababel/concurrent-http-server.git
cd concurrent-http-server
make
./server        # listens on port 28281
```

Then, using the included client, in another terminal:
```bash
echo "GET index.html" | ./client localhost
```

### Verify memory safety
```bash
valgrind --leak-check=full --trace-children=yes ./server
```

## 🧠 What I learned

- The Unix sockets lifecycle and request/response handling at the byte level
- Process-based concurrency and the zombie-process problem (and how to solve it)
- Using Valgrind to prove memory safety across forked child processes under concurrent load

## 📄 License

MIT
