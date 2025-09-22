# Collaborative Markdown Editor (C)

## Overview
This project implements a **collaborative, version-controlled Markdown editor** in C using a client–server architecture.  
It supports multiple clients editing the same shared document concurrently, with changes queued, validated, and applied in consistent versions.

Key features:
- **Custom document model**: linked-list–based text storage with version tracking.
- **Deferred change application**: edits are queued and applied atomically on version increment.
- **Markdown operations**: insert, delete, headings, bold/italic, lists, links, horizontal rules, and more.
- **Client–server communication**: POSIX **FIFOs** (named pipes) and **signals** for synchronization.
- **Concurrency**: server spawns a thread per client using **POSIX threads**.
- **Event loop**: server uses `epoll` to handle I/O efficiently.
- **Logging & recovery**: command log is maintained; server can broadcast updated documents.

---

## Repository Structure
├── src/
│ ├── client.c # Client program
│ ├── server.c # Server program
│ ├── commands.c # Command parsing and validation
│ ├── markdown.c # Document operations and change application
│
├── include/
│ ├── commands.h
│ ├── document.h
│ ├── markdown.h
│ ├── server.h
│
├── Makefile # Build configuration
└── README.md

---

## Build Instructions
Ensure you are on a Unix-like system with GCC and pthreads available.

```bash
# Compile both server and client
make

# Or build separately
make server
make client

This will produce two binaries:
1. server
2. client

## Usage
1. Start the Server
./server <interval>


<interval> = number of seconds between version increments (applies queued changes).

Example:

./server 5


Starts the server with a 5-second commit interval.

2. Start a Client
./client <serverPID> <username>


<serverPID> = the PID of the running server (printed on startup).

<username> = unique identifier for the client.

Example:

./client 12345 alice

3. Supported Commands

Clients can send text-based commands. Examples include:

Insert text:

INSERT 1 5 hello


Delete a range:

DELETE 2 10


Apply formatting:

BOLD 1 4
HEADING 2 1
LINK 5 10 https://example.com


View the current document or logs:

DOC?
LOG?

Technical Highlights

Systems programming: raw POSIX APIs (epoll, signals, FIFOs, pthreads).

Data structures: linked lists, queues, and custom change objects.

Error handling: robust parsing, input validation, and safe memory management.

Scalability: deferred edit application avoids race conditions and supports multiple users.

Future Improvements

Authentication and user roles.

Persistent storage of document versions.

Networking over TCP sockets (instead of FIFOs).

Richer Markdown support.
