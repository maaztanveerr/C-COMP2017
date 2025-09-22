#ifndef MARKDOWN_H
#define MARKDOWN_H
#include <stdio.h>
#include <stdint.h>
#include "document.h"  

// defined extra stuff in the header
// defining change types for pending changes meaning if its an insert change, delete change etc
#define CHANGE_INSERT 1
#define CHANGE_DELETE 2
#define CHANGE_BOLD 3
#define CHANGE_ITALIC 4
#define CHANGE_BLOCKQUOTE 5
#define CHANGE_ORDERED_LIST 6
#define CHANGE_UNORDERED_LIST 7
#define CHANGE_CODE 8
#define CHANGE_HORIZONTAL_RULE 9
#define CHANGE_LINK 10
#define CHANGE_NEWLINE 11
#define CHANGE_HEADING 12

// now structure to store change until version is updated
// structure to store a change until version is updated
typedef struct changeNode
{
    int type;                // type of change using defines above
    size_t pos;              // position in document
    size_t end;              // end position for range functions
    size_t level;            // level for headings
    char *content;           // content for insert or link url
    struct changeNode *next; // link to next change
} changeNode;


/**
 * The given file contains all the functions you will be required to complete. You are free to and encouraged to create
 * more helper functions to help assist you when creating the document. For the automated marking you can expect unit tests
 * for the following tests, verifying if the document functionalities are correctly implemented. All the commands are explained 
 * in detail in the assignment spec.
 */

// Initialize and free a document
document * markdown_init(void); 
void markdown_free(document *doc);

// === Edit Commands ===
int markdown_insert(document *doc, uint64_t version, size_t pos, const char *content);
int markdown_delete(document *doc, uint64_t version, size_t pos, size_t len);

// === Formatting Commands ===
int markdown_newline(document *doc, size_t version, size_t pos);
int markdown_heading(document *doc, uint64_t version, size_t level, size_t pos);
int markdown_bold(document *doc, uint64_t version, size_t start, size_t end);
int markdown_italic(document *doc, uint64_t version, size_t start, size_t end);
int markdown_blockquote(document *doc, uint64_t version, size_t pos);
int markdown_ordered_list(document *doc, uint64_t version, size_t pos);
int markdown_unordered_list(document *doc, uint64_t version, size_t pos);
int markdown_code(document *doc, uint64_t version, size_t start, size_t end);
int markdown_horizontal_rule(document *doc, uint64_t version, size_t pos);
int markdown_link(document *doc, uint64_t version, size_t start, size_t end, const char *url);

// === Utilities ===
void markdown_print(const document *doc, FILE *stream);
char *markdown_flatten(const document *doc);

// === Versioning ===
void markdown_increment_version(document *doc);

// === Document Helper Functions ===
// These functions perform actual document modifications without queueing
int docInserter(document *doc, size_t pos, const char *content);
int docDeleter(document *doc, size_t pos, size_t len);
int docNewline(document *doc, size_t pos);
int docHeading(document *doc, size_t level, size_t pos);
int docBold(document *doc, size_t start, size_t end);
int docItalic(document *doc, size_t start, size_t end);
int docBlockquote(document *doc, size_t pos);
int docOrderedlist(document *doc, size_t pos);
int docUnorderedlist(document *doc, size_t pos);
int docCode(document *doc, size_t start, size_t end);
int docHorizontalRule(document *doc, size_t pos);
int docLink(document *doc, size_t start, size_t end, const char *url);

#endif // MARKDOWN_H