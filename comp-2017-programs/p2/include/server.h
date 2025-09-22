#ifndef SERVER_H
#define SERVER_H

// standard library includes
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>  //for signal functions
#include <pthread.h> // for pthread create
#include <string.h>
#include <sys/stat.h> // for mkfifo
#include <fcntl.h>
#include <stdbool.h>
#include <sys/epoll.h> // for epoll
#include <time.h>
 
// other relevant files
#include "markdown.h"
#include "commands.h"

// constants
#define MAX_CLIENTS 100    // can have a max of 100 clients at one time
#define MAX_QCOMMANDS 1000 // can have a max of 1000 commands in queue at one time

// Structure definitions
// client information structure
typedef struct clientinfo
{
    pid_t pid;       // client process id
    int writingfd;   // file descriptor for writing to client
    int readingfd;   // fd for reading from client
    int permissions; // read or write
    bool connected;  // whether connected or not
} clientinfo;

// command queue structure
typedef struct commandQ
{
    command commands[MAX_QCOMMANDS]; // actual commands
    char *commandstr[MAX_QCOMMANDS]; // commands in string form
    int count;                       // num of commands in queue
} commandQ;

// function declarations
// Client handling functions
void clean(int read, int write);
void *clienthandler(void *arg);
void addClient(pid_t pid, int writingfd, int readingfd, int permissions);
void disconnectClient(pid_t pid);

// command processing functions
void qcommandprocessor(void);
void broadcasting(uint64_t version, char **responses, int resCount);

// signal handling function
void signalHandler(int signal, siginfo_t *info, void *status);

#endif // SERVER_H