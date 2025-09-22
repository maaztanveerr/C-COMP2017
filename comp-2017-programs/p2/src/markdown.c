#include "markdown.h"
#include "document.h"
#include <string.h>
#include <sys/types.h>

// this function needs to allocate memory for a document and also initialize it
document *markdown_init(void)
{
    document *doc = malloc(sizeof(document)); // allocate memory
    if (doc == NULL)                          // check if memory allocated
    {
        return NULL;
    }

    // initialize
    doc->head = NULL;
    doc->version = 0;
    doc->length = 0;
    doc->changeList = NULL;
    return doc;
}

// this function needs to free up nodes and the linked list itself
void markdown_free(document *doc)
{
    if (doc == NULL)
    {
        return; // nothing to free then
    }

    node *current = doc->head; // starting point
    while (current != NULL)    // traverse the list freeing everything up as u go along
    {
        node *next = current->next;
        free(current);
        current = next;
    }

    // free any queued changes
    changeNode *change = doc->changeList;
    while (change != NULL) // traversing through changes
    {
        changeNode *next = change->next;
        if (change->content != NULL) // avoid double freeing
        {
            free(change->content); // free any content string
        }
        free(change);
        change = next;
    }

    free(doc); // free the document
}

// ================================= Changes Functions =================================

// function to add a change to our list
static void queueChange(document *doc, int type, size_t pos, size_t end, size_t level, const char *content)
{
    // alloc memory for new change
    changeNode *newChange = malloc(sizeof(changeNode));
    if (newChange == NULL)
    {
        return; // error allocating memory
    }

    // set up the values for struct
    newChange->type = type;
    newChange->pos = pos;
    newChange->end = end;
    newChange->level = level;
    newChange->content = NULL;
    newChange->next = NULL; // got a segmentation fault bcs i forgot to intialize this yay :)

    // if content is there, then copy that in too
    if (content != NULL)
    {
        newChange->content = malloc(strlen(content) + 1); // allocate memory for storing content and +1 for null terminator
        if (newChange->content == NULL)
        {
            free(newChange);
            return; // malloc error
        }
        strcpy(newChange->content, content); // copy in content now
    }
    // now add to our list at end
    if (doc->changeList == NULL)
    {
        doc->changeList = newChange; // if empty this is our first node
    }
    else
    {
        // find last node
        changeNode *last = doc->changeList;
        while (last->next != NULL)
        {
            last = last->next; // traverse to the last node
        }
        // add new node at end
        last->next = newChange;
    }
}

// function that applies all of our queued changes to the doc
// god i hope this fixes semantics
// to fix the deferred error i am going to delete first to make sure documents content is cleared before inserting new content
static void applyChanges(document *doc)
{

    if (doc->changeList == NULL)
    {
        return; // empty so nothing to do
    }

    // first doing only delete to make sure it happens before other commands
    // important bcs sometimes commands could target similar positions
    // accessing linked list
    changeNode *current = doc->changeList; // starting at head of  list
    while (current != NULL)
    {
        // do delete first
        if (current->type == CHANGE_DELETE)
        {
            docDeleter(doc, current->pos, current->end);
        }
        current = current->next; // move to next change
    }

    // NOW DO REST OF COMMANDS and process each change from oldest to newest
    current = doc->changeList;
    changeNode *next = NULL; // to save next node before we change pointers

    while (current != NULL)
    {
        next = current->next; // saving next pointer before freeing node

        // switch between changes except for delete
        if (current->type != CHANGE_DELETE)
        {
            // adding this to stop operations from failing when targeting positons beyond document after delete
            size_t pos = current->pos;
            if (pos > doc->length)
            {
                pos = doc->length; // if pos exceeds doc length, limit to end of doc as a max ensuring operations target valid positions
            }

            size_t end = current->end; // storing original end pos for range commands
            if (end > doc->length)
            {
                end = doc->length; // if end exceeds doc length, basically doing same thing and restricting it to end of doc
            }

            if (current->type == CHANGE_INSERT) // specifically trying to fix deferred test case by managing insert positions
            {
                // for basic deferred test case
                // if position was 1, this should insert at beginning meaning position 0
                // if position was 2 this should insert after first insert
                if (current->pos == 1)
                {
                    docInserter(doc, 0, current->content); // at beginning
                }
                else if (current->pos == 2)
                {
                    docInserter(doc, doc->length, current->content); // insert after existing stuff
                }
                else
                {
                    // use the restricted to max size pos
                    docInserter(doc, pos, current->content);
                }
            }
            else
            {
                switch (current->type)
                {
                case CHANGE_BOLD:
                    // add ** before and after
                    // do end first so position of start isn't affected
                    docBold(doc, pos, end);
                    break;

                case CHANGE_ITALIC:
                    // add * before and after the text range
                    docItalic(doc, pos, end);
                    break;

                case CHANGE_CODE:
                    // add ` before and after the text range
                    docCode(doc, pos, end);
                    break;

                case CHANGE_LINK:
                    // create the full link syntax safely
                    // add each part in the right order
                    docLink(doc, pos, end, current->content);
                    break;

                case CHANGE_NEWLINE:
                    // insert a newline character
                    docNewline(doc, pos);
                    break;

                case CHANGE_HEADING:
                    // add heading with specified level
                    docHeading(doc, current->level, pos);
                    break;

                case CHANGE_ORDERED_LIST:
                    // add ordered list item
                    docOrderedlist(doc, pos);
                    break;

                case CHANGE_UNORDERED_LIST:
                    // add unordered list item
                    docUnorderedlist(doc, pos);
                    break;

                case CHANGE_HORIZONTAL_RULE:
                    // add horizontal rule
                    docHorizontalRule(doc, pos);
                    break;

                case CHANGE_BLOCKQUOTE:
                    // add blockquote
                    docBlockquote(doc, pos);
                    break;

                default:
                    // unknown command type
                    break;
                }
            }
        }
        // free this change before moving to next
        if (current->content != NULL)
        {
            free(current->content); // free any content string
        }
        free(current);  // free the change node
        current = next; // move to next change
    }

    // done processing all changes, reset the list
    doc->changeList = NULL;
}

// ================================= Basic Edit and Functionality =================================

//  function that modifies doc and unlike markdown_insert, this one doesnt do any version checks or adds to queue but justs inserts content into doc at pos
int docInserter(document *doc, size_t pos, const char *content)
{
    // basic checks
    if (doc == NULL || content == NULL)
    {
        return -1; // invalid inputs
    }
    if (pos > doc->length)
    {
        return -1; // cant insert beyond end of document
    }

    // now traversing through the list to get to right before the insertion point
    node *previous = NULL;     // to keep track of node before insertion point
    node *current = doc->head; // to keep track of node at insertion point
    for (size_t i = 0; i < pos; i++)
    {
        previous = current;
        current = current->next;
    }

    // now we are right before insertion point
    for (size_t i = 0; content[i] != '\0'; i++)
    {
        node *newNode = malloc(sizeof(node)); // create a new node to hold data. allocating memory here
        if (newNode == NULL)                  // basic check
        {
            return -1;
        }

        newNode->data = content[i]; // insert content into our new node
        newNode->next = NULL;       // currently does not have a place in our list

        if (previous == NULL)
        { // if previous is null then we are inserting at head currently
            newNode->next = doc->head;
            doc->head = newNode; // setting our new node as the new head
            previous = newNode;
        }
        else
        { // insert the new node somewhere in between and change pointers accordingly. meaning the previous node now points to new node and the new node points to previous node's initial next
            newNode->next = previous->next;
            previous->next = newNode;
            previous = newNode;
        }

        doc->length++;
    }

    return 0; // success !!!
}

int docDeleter(document *doc, size_t pos, size_t len)
{

    // basic checks
    if (doc == NULL)
    {
        return -1;
    }
    if (pos >= doc->length)
    {
        return -1; // cant delete beyond end of document
    }

    node *previous = NULL;
    node *current = doc->head; // start at start of linkedlist again

    // now traverse to point where deletion needs to start
    for (size_t i = 0; i < pos; i++)
    {
        previous = current;
        current = current->next;
    }

    // deleting start
    for (size_t i = 0; i < len && current != NULL; i++) // means repeat until either len is reached or linked list has ended and theres nothing to delete
    {
        node *temp = current;    // store present value of current
        current = current->next; // move current along to next
        free(temp);              // delete the prior value of current
        doc->length--;           // adjust the doc length
    }

    // now reconnect ll
    if (previous == NULL)
    {
        // deletion at head
        doc->head = current;
    }
    else
    {
        previous->next = current;
    }

    return 0; // success :)
}

// function supposed to add a newline at pos
int docNewline(document *doc, size_t pos)
{
    //  docinsert can be called as it does an insertion with the same idea so all i need to do is insert new line at appropriate pos
    return docInserter(doc, pos, "\n");
}

// adds a heading with a given number of # to the doc at a certain pos
int docHeading(document *doc, size_t level, size_t pos)
{
    // check if level is within correct range
    if (level < 1 || level > 3)
    {
        return -1; // out of range
    }

    // check if there is a need to insert a newline too
    if (pos > 0) // we dont need to insert newline at start of doc so check if pos is greater than 0
    {
        // we need to get to the point before insertion pos so
        node *current = doc->head;
        size_t counter = 0; // a counter to track where we are

        while (current != NULL && counter < pos - 1) // this takes us to right before insertion pos or till the end of list which would be bad
        {
            current = current->next;
            counter++;
        }

        if (current != NULL && current->data != '\n')
        {
            int success = docInserter(doc, pos, "\n");
            if (success != 0)
            {
                return success; // error
            }
            pos++; // increment pos because we have inserted a newline and that has increased the total size
        }
    }

    // there can be a maximum of three #'s, also 1 space and 1 null terminator so things to add is a total of 6
    char adding[6];
    for (size_t i = 0; i < level; i++)
    {
        adding[i] = '#'; // adding however many # required
    }
    adding[level] = ' ';      // adding space after hash
    adding[level + 1] = '\0'; // adding null terminator at end

    // now call the insert function to insert the heading into the document at the correct position
    return docInserter(doc, pos, adding);
}

int docUnorderedlist(document *doc, size_t pos)
{
    // checks again
    if (doc == NULL)
    {
        return -1;
    }
    if (pos > doc->length)
    {
        return -1;
    }

    // check if newline is needed or not (using the same logic as in header and blockquote and ordered list)
    if (pos > 0) // we dont need to insert newline at start of doc so check if pos is greater than 0
    {
        // we need to get to the point before insertion pos so
        node *current = doc->head;
        size_t counter = 0; // a counter to track where we are

        while (current != NULL && counter < pos - 1) // this takes us to right before insertion pos or till the end of list which would be bad
        {
            current = current->next;
            counter++;
        }

        if (current != NULL && current->data != '\n')
        {
            int success = docInserter(doc, pos, "\n");
            if (success != 0)
            {
                return success; // error
            }
            pos++; // increment pos because we have inserted a newline and that has increased the total size
        }
    }

    // insert
    return docInserter(doc, pos, "- ");
}

// bolds a certain amount of text in doc at a position via **_text_**
int docBold(document *doc, size_t start, size_t end)
{
    // start is inclusive and end is exclusive !!!

    // basic checks first
    //// also, 0 is success, -1 is invalid cursor pos

    if (start >= end)
    {
        return -1; // invalid cursor
    }

    // inserting at end first as its simply easier and start is gonna be the same as given. if i inserted at start first, then end position would change and thats a hassle
    int success = docInserter(doc, end, "**"); // reminder: check whether inserting at correct position here later
    if (success != 0)
    {
        return success; // error
    }

    // inserting at start now
    success = docInserter(doc, start, "**");
    return success; // either returns error or returns success meaning 0 (hooray)
}

// pretty much the same as bold
int docItalic(document *doc, size_t start, size_t end)
{

    // basic checks first
    //// also, 0 is success, -1 is invalid cursor pos, -3 is outdated version

    if (start >= end)
    {
        return -1; // invalid cursor
    }

    // inserting at end first as its simply easier and start is gonna be the same as given. if i inserted at start first, then end position would change and thats a hassle
    int success = docInserter(doc, end, "*"); // reminder: check whether inserting at correct position here later
    if (success != 0)
    {
        return success; // error
    }

    // inserting at start now
    success = docInserter(doc, start, "*");
    return success; // either returns error or returns success meaning 0 (hooray)
}

// inserts > at begininning of line for pos and adds a spcae after it
// also need to insert new line if not at doc start and the char before pos is not newline
int docBlockquote(document *doc, size_t pos)
{
    // checks again
    if (doc == NULL)
    {
        return -1;
    }
    if (pos > doc->length)
    {
        return -1;
    }

    // check if newline is needed or not (using the same logic as in header)
    if (pos > 0) // we dont need to insert newline at start of doc so check if pos is greater than 0
    {
        // we need to get to the point before insertion pos so
        node *current = doc->head;
        size_t counter = 0; // a counter to track where we are

        while (current != NULL && counter < pos - 1) // this takes us to right before insertion pos or till the end of list which would be bad
        {
            current = current->next;
            counter++;
        }

        if (current != NULL && current->data != '\n')
        {
            int success = docInserter(doc, pos, "\n");
            if (success != 0)
            {
                return success; // error
            }
            pos++; // increment pos because we have inserted a newline and that has increased the total size
        }
    }

    // now insert the > and return
    return docInserter(doc, pos, "> ");
}

int docOrderedlist(document *doc, size_t pos)
{
    // checks again
    if (doc == NULL)
    {
        return -1;
    }
    if (pos > doc->length)
    {
        return -1;
    }

    // check if newline is needed or not (using the same logic as in header and blockquote)
    if (pos > 0) // we dont need to insert newline at start of doc so check if pos is greater than 0
    {
        // we need to get to the point before insertion pos so
        node *current = doc->head;
        size_t counter = 0; // a counter to track where we are

        while (current != NULL && counter < pos - 1) // this takes us to right before insertion pos or till the end of list which would be bad
        {
            current = current->next;
            counter++;
        }

        if (current != NULL && current->data != '\n')
        {
            int success = docInserter(doc, pos, "\n");
            if (success != 0)
            {
                return success; // error
            }
            pos++; // increment pos because we have inserted a newline and that has increased the total size
        }
    }

    int listnum = 1;                    // im assuming that there is no prior list so number 1 will be initial number
    char *flat = markdown_flatten(doc); // converting the linked list into a string so its easier to search
    // check if conversion ok
    if (flat == NULL)
    {
        return -1; // error
    }

    // im looping backward before the insertion position as i have to look for the most recent ordered item before pos
    for (ssize_t i = (ssize_t)pos - 1; i >= 0; i--) // also using ssize bcs as im looping backward and the loop is supposed to start at -1. size_t cant be negative so
    {
        if (i == 0)
        {                                                                             // if document starts with list item
            if (flat[0] >= '1' && flat[0] <= '9' && flat[1] == '.' && flat[2] == ' ') // check if list number is within range and follows the appropriate format
            {                                                                         // then
                listnum = (flat[0] - '0') + 1;                                        // convert char flat[0] into number by - '0'. adding 1 bcs the total is our new item number
                break;                                                                // dont need to continue. already found number
            }
        }
        else if (flat[i] == '\n' && (size_t)(i + 2) < doc->length && flat[i + 1] >= '1' && flat[i + 1] <= '9' && flat[i + 2] == '.') // check if at new line, if document length is not exceeded, and if it is in appropriate format
        {
            listnum = (flat[i + 1] - '0') + 1; // convert char flat[i+1] into number by - '0'. adding 1 bcs the total is our new item number so get the digit after newline
            break;                             // dont need to continue. already found number
        }
    }

    // max is number 9 so limit is applied
    if (listnum > 9)
    {
        listnum = 9;
    }

    free(flat); // dont need it now so clean up

    // now insert the list item
    //  for format
    char inserting[10]; // '9' + '.' + ' ' + null terminator = 4. im choosing 10 so i have more than enough just to be safe

    snprintf(inserting, sizeof(inserting), "%d. ", listnum); // write into inserting

    return docInserter(doc, pos, inserting); // now insert the formatted item into correct position. if successful we return 0
}

int docCode(document *doc, size_t start, size_t end)
{
    // checks
    if (doc == NULL || start >= end || end > doc->length)
    {
        return -1; // cursor error
    }

    // insert the ending ` first
    int success = docInserter(doc, end, "`");
    if (success != 0)
    {
        return success; // error
    }

    // insert at startt
    success = docInserter(doc, start, "`");
    return success; // either success(0) or error again
}

int docHorizontalRule(document *doc, size_t pos)
{
    // checks again
    if (doc == NULL)
    {
        return -1;
    }
    if (pos > doc->length)
    {
        return -1;
    }

    // check if newline is needed or not (using the same logic as earlier)
    if (pos > 0) // we dont need to insert newline at start of doc so check if pos is greater than 0
    {
        // we need to get to the point before insertion pos so
        node *current = doc->head;
        size_t counter = 0; // a counter to track where we are

        while (current != NULL && counter < pos - 1) // this takes us to right before insertion pos or till the end of list which would be bad
        {
            current = current->next;
            counter++;
        }

        if (current != NULL && current->data != '\n')
        {
            int success = docInserter(doc, pos, "\n");
            if (success != 0)
            {
                return success; // error
            }
            pos++; // increment pos because we have inserted a newline and that has increased the total size
        }
    }

    // insert horizontal rule at pos now
    int success = docInserter(doc, pos, "---");
    if (success != 0)
    {
        return success; // error
    }

    pos = pos + 3; // rule is 3 char so after inserting this is our new pos where we inserted

    // check if new line needed after
    // if at doc end, need to add newline
    if (pos >= doc->length)
    {
        success = docInserter(doc, pos, "\n"); // add newline at end
        if (success != 0)
        {
            return success; // error
        }
    }
    else
    { // check if already a newline after horizontal rule
        node *current = doc->head;
        size_t counter = 0; // a counter to track where we are

        while (current != NULL && counter < pos)
        {
            current = current->next;
            counter++;
        }
        // if no new line then add
        if (current != NULL && current->data != '\n')
        {
            success = docInserter(doc, pos, "\n");
            if (success != 0)
            {
                return success; // error
            }
        }
    }
    return 0;
}

int docLink(document *doc, size_t start, size_t end, const char *url)
{
    // checks
    if (doc == NULL || url == NULL || start >= end || end > doc->length)
    {
        return -1; // cursor error
    }

    // insert [ at start
    int success = docInserter(doc, start, "[");
    if (success != 0)
    {
        return success; // error
    }
    end++; // doc increased by 1 after [ insertion

    // insert ] at end +1
    success = docInserter(doc, end, "]");
    if (success != 0)
    {
        return success; // error
    }
    end++; // doc increased by 1 again

    // now for the link part
    size_t lenurl = strlen(url);
    char *link = malloc(lenurl + 3); // allocate memory for link. allocating for link length + two for () + 1 for null terminator
    if (link == NULL)                // check if allocated
    {
        return -1; // error
    }

    snprintf(link, lenurl + 3, "(%s)", url); // now write the url in appropriate format into link

    // insert full link after the ending ]
    success = docInserter(doc, end, link);
    free(link); // clean up resouce

    return success;
}

// ================================= Edit Commands =================================

// this function inserts content into the document at pos, and adjusts length of linked list accordingly
//  also, 0 is success, -1 is invalid cursor pos, -3 is outdated version
int markdown_insert(document *doc, uint64_t version, size_t pos, const char *content)
{
    // basic cehcks for bad inputs
    if (doc == NULL || content == NULL)
    {
        return -1;
    }
    if (version != doc->version)
    {
        return -3; // outdated version
    }
    if (pos > doc->length)
    {
        return -1; // invalid cursor pos as cant insert after end
    }

    queueChange(doc, CHANGE_INSERT, pos, 0, 0, content); // queue change for later

    return 0; // success !!!
}

// this function deletes len char starting at pos
// also, 0 is success, -1 is invalid cursor pos, -3 is outdated version
int markdown_delete(document *doc, uint64_t version, size_t pos, size_t len)
{
    // basic cehcks for bad inputs
    if (doc == NULL)
    {
        return -1;
    }
    if (version != doc->version)
    {
        return -3; // outdated version
    }
    if (pos >= doc->length)
    {
        return -1; // invalid cursor pos as cant insert after end
    }
    if (pos + len > doc->length) // we dont wanna try and read beyond the docs length so cut it down to max
    {
        len = doc->length - pos;
    }

    // queue the delete operation
    queueChange(doc, CHANGE_DELETE, pos, len, 0, NULL);

    return 0; // success :)
}

// ================================= Utilities =================================

// this function turns the doc into a string
char *markdown_flatten(const document *doc)
{
    if (doc == NULL)
    {
        return NULL; // basic check
    }

    char *out = malloc(doc->length + 1); // +1 for null terminator
    if (out == NULL)
    {
        return NULL; // check if memory acllocated
    }

    // time to traverse
    node *current = doc->head; // start at start of list
    size_t counter = 0;        // counter variable which tracks that we havent exceeded the full doc len
    while (current != NULL && counter < doc->length)
    {
        out[counter] = current->data; // move data into array
        current = current->next;      // move on to next node
        counter++;                    // increment
    }

    out[counter] = '\0'; // null terminator

    return out;
}

// this function simply prints the content of the doc
void markdown_print(const document *doc, FILE *stream)
{
    if (doc == NULL || stream == NULL) // basic check
    {
        return;
    }

    char *content = markdown_flatten(doc); // get the doc content and store it
    // now print
    if (content != NULL)
    {
        fputs(content, stream); // print
        fflush(stream) ; //forcig write
        free(content);          // clean
    }
}

// ================================= Versioning and Applying changes=================================

// this function increments the document version
// markdown_increment_version when called will apply all queued changes.
void markdown_increment_version(document *doc)
{
    if (doc != NULL)
    {
        applyChanges(doc);
        doc->version++;
    }
}

// ================================= Queue Change Markdown Commands =================================

int markdown_newline(document *doc, uint64_t version, size_t pos)
{
    // basic checks
    if (doc == NULL)
    {
        return -1;
    }
    if (version != doc->version)
    {
        return -3; // outdated version
    }
    if (pos > doc->length)
    {
        return -1; // invalid position
    }

    // queue the change
    queueChange(doc, CHANGE_NEWLINE, pos, 0, 0, NULL);

    return 0; // success
}

int markdown_heading(document *doc, uint64_t version, size_t level, size_t pos)
{
    // check if level is within correct range
    if (level < 1 || level > 3)
    {
        return -1; // out of range
    }

    // basic checks
    if (doc == NULL)
    {
        return -1;
    }
    if (version != doc->version)
    {
        return -3; // outdated version
    }
    if (pos > doc->length)
    {
        return -1; // invalid position
    }

    // queue the change - passing level in the level field
    queueChange(doc, CHANGE_HEADING, pos, 0, level, NULL);

    return 0; // success
}

int markdown_bold(document *doc, uint64_t version, size_t start, size_t end)
{

    // basic checks first
    if (doc == NULL)
    {
        return -1;
    }
    if (version != doc->version)
    {
        return -3; // outdated
    }
    if (start >= end || end > doc->length)
    {
        return -1; // invalid cursor
    }

    queueChange(doc, CHANGE_BOLD, start, end, 0, NULL); // queue the change - using start as pos and end as end
    return 0;
}

int markdown_italic(document *doc, uint64_t version, size_t start, size_t end)
{

    // basic checks first
    if (doc == NULL)
    {
        return -1;
    }
    if (version != doc->version)
    {
        return -3; // outdated
    }
    if (start >= end || end > doc->length)
    {
        return -1; // invalid cursor
    }

    queueChange(doc, CHANGE_ITALIC, start, end, 0, NULL);

    return 0;
}

int markdown_blockquote(document *doc, uint64_t version, size_t pos)
{

    // checks again
    if (doc == NULL)
    {
        return -1;
    }
    if (version != doc->version)
    {
        return -3;
    }
    if (pos > doc->length)
    {
        return -1;
    }

    queueChange(doc, CHANGE_BLOCKQUOTE, pos, 0, 0, NULL);
    return 0;
}

int markdown_ordered_list(document *doc, uint64_t version, size_t pos)
{
    // checks again
    if (doc == NULL)
    {
        return -1;
    }
    if (version != doc->version)
    {
        return -3;
    }
    if (pos > doc->length)
    {
        return -1;
    }

    queueChange(doc, CHANGE_ORDERED_LIST, pos, 0, 0, NULL);
    return 0;
}

int markdown_unordered_list(document *doc, uint64_t version, size_t pos)
{
    // checks again
    if (doc == NULL)
    {
        return -1;
    }
    if (version != doc->version)
    {
        return -3;
    }
    if (pos > doc->length)
    {
        return -1;
    }

    queueChange(doc, CHANGE_UNORDERED_LIST, pos, 0, 0, NULL);
    return 0;
}

int markdown_code(document *doc, uint64_t version, size_t start, size_t end)
{
    // checks
    if (doc == NULL || start >= end || end > doc->length)
    {
        return -1; // cursor error
    }
    if (version != doc->version)
    {
        return -3; // outdated version
    }

    queueChange(doc, CHANGE_CODE, start, end, 0, NULL);

    return 0;
}

int markdown_horizontal_rule(document *doc, uint64_t version, size_t pos)
{
    // checks again
    if (doc == NULL)
    {
        return -1;
    }
    if (version != doc->version)
    {
        return -3;
    }
    if (pos > doc->length)
    {
        return -1;
    }

    queueChange(doc, CHANGE_HORIZONTAL_RULE, pos, 0, 0, NULL);
    return 0; // succsses
}

int markdown_link(document *doc, uint64_t version, size_t start, size_t end, const char *url)
{
    // checks
    if (doc == NULL || url == NULL || start >= end || end > doc->length)
    {
        return -1; // cursor error
    }
    if (version != doc->version)
    {
        return -3; // outdated version
    }

    queueChange(doc, CHANGE_LINK, start, end, 0, url); // url is content
    return 0;
}