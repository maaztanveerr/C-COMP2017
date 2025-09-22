#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <stdint.h>
#include <stdlib.h>

//utilizing a linked list

typedef struct node {
    char data;      // storing one char in doc
    struct node *next;  //pointer to next char in doc
} node; 

//structure for document
typedef struct document {
    node *head; //start of doc
    uint64_t version; // version number (uint64 is spec requirement)
    size_t length; //total num of char
    struct changeNode *changeList; // track queued changes
} document;

#endif