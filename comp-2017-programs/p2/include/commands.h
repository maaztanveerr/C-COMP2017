#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h> // uint64
#include <stdlib.h>
#include "markdown.h"

// macros
#define FAIL 0                   
#define READ 1
#define WRITE 2
#define MAX_LOG_ENTRIES 1000  // big num for lots of log entries

//commands type
#define INSERT_COM 1
#define DELETE_COM 2
#define NEWLINE_COM 3
#define HEADING_COM 4
#define BOLD_COM 5
#define ITALIC_COM 6
#define BLOCKQUOTE_COM 7
#define ORDEREDLIST_COM 8
#define UNORDEREDLIST_COM 9
#define CODE_COM 10
#define HORIZONTALRULE_COM 11
#define LINK_COM 12
#define DISCONNECT_COM 13
#define DOC_COM 14
#define PERM_COM 15
#define LOG_COM 16

// defining command structure to hold command info
typedef struct command {
    int type; //command type
    uint64_t version; //doc version
    size_t pos; // position
    size_t end; // last position within a range
    size_t level; // heading level
    char *content; // actual content
} command;

// struct for holding 1 log entry with result and doc version
typedef struct logentry {
    char *responseStr;  // result of command execution
    uint64_t version;    // ver num for grouping entries
} logentry;

// struct to hold all log entries from commands
typedef struct commandlog {
    logentry entries[MAX_LOG_ENTRIES];  // array of all entries
    int count;                          // how many entries we got
} commandlog;

// Main command functions
int parseCom(const char *strCom, command *cmd, int permissions);
char* executeCom(command *cmd, document *doc, int permissions);
void freeCom(command *cmd);

// Log management functions
commandlog* initializeLog(void);
void logCom(commandlog *log, const char *response, uint64_t version);
char* getLogReport(commandlog *log);
void freeLog(commandlog *log);

// Helper parsing functions
int parsingHelp(const char *arr, size_t *index, long *out, size_t max);
int parseVersionPos(const char *strCom, command *cmd, int type, char *temp, size_t tempSize, size_t *i);
int parseInsertCom(const char *strCom, command *cmd);
int parseDelCom(const char *strCom, command *cmd);
int parseHeadingCom(const char *strCom, command *cmd);
int parseLinkCom(const char *strCom, command *cmd);
int parseOnePosCom(const char *strCom, command *cmd, int type);
int parseTwoPosCom(const char *strCom, command *cmd, int type);

#endif // COMMANDS_H