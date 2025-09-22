#include <string.h>
#include <stdio.h>
#include "commands.h"
#include "markdown.h"
 
// ================================= PARSING STUFF =================================

int parsingHelp(const char *arr, size_t *index, long *out, size_t max) // check later !!!
{
    while (*index < max && arr[*index] == ' ')
    {
        (*index)++; // code to skip spaces
    }

    if (*index >= max || arr[*index] == '\0')
    {
        return -1; // means that we reached the end without finding a value
    }

    // now actually parse the string
    char *end; // to keep track of where we stopped after parsing
    long val = strtol(&arr[*index], &end, 10);

    // if negative val or if nothing was parsed and we are at end of array
    if (val < 0 || end == &arr[*index])
    {
        return -1; // error
    }

    // else set out as val
    *out = val;

    // // move further ahead accordingly
    *index = end - arr;
    return 0; // success
}

int parseVersionPos(const char *strCom, command *cmd, int type, char *temp, size_t tempSize, size_t *i)
{
    strncpy(temp, strCom, tempSize - 1); // copy command into temp
    temp[tempSize - 1] = '\0';           // null terminator

    // now we need to skip the command from string to get to version and pos
    *i = 0; // index

    cmd->type = type;

    while (temp[*i] != ' ' && temp[*i] != '\0')
    {
        (*i)++; // moving until we find either a space indicating the end of the word for command or either the end of string
    }

    // parse version number
    long version;
    int success = parsingHelp(temp, i, &version, tempSize - 1);
    if (success != 0)
    {
        return -1; // error
    }
    cmd->version = version;

    // parse pos number
    long position;
    success = parsingHelp(temp, i, &position, tempSize - 1);
    if (success != 0)
    {
        return -1; // error
    }
    cmd->pos = position;

    // code to skip every space afterwards
    while (temp[*i] == ' ')
    {
        (*i)++;
    }

    return 0;
}

int parseInsertCom(const char *strCom, command *cmd)
{
    char temp[256]; // temp array which will hold string for a bit
    // now we need to skip the command from string to get to version and pos
    size_t i = 0; // index variable

    if (parseVersionPos(strCom, cmd, INSERT_COM, temp, sizeof(temp), &i) != 0)
    {
        return -1;
    }

    // now we do content
    // whatevers left after this is all content
    if (temp[i] == '\0')
    {
        // reached end no content
        cmd->content = malloc(1);
        if (cmd->content == NULL)
        {
            return -1;
        }
        cmd->content[0] = '\0'; // there is no content so set that
    }
    else
    {
        size_t len = strlen(&temp[i]);
        cmd->content = malloc(len + 1);
        if (cmd->content == NULL)
        {
            return -1;
        }
        strcpy(cmd->content, &temp[i]);
    }

    for (size_t f = 0; cmd->content[f] != '\0'; f++)
    {
        if (cmd->content[f] == '\n')
        {
            free(cmd->content);
            return -1; // content with new line is an error as it is not allowed per spec
        }
    }

    return 0; // success!
}

int parseDelCom(const char *strCom, command *cmd)
{
    char temp[256]; // temp array which will hold string for a bit
    size_t i = 0;   // index variable

    if (parseVersionPos(strCom, cmd, DELETE_COM, temp, sizeof(temp), &i) != 0)
    {
        return -1;
    }

    // parse length (storing it in cmd.end)
    long len;
    int success = parsingHelp(temp, &i, &len, sizeof(temp) - 1);
    if (success != 0)
    {
        return -1; // error
    }
    cmd->end = len;

    return 0;
}

int parseHeadingCom(const char *strCom, command *cmd)
{
    char temp[256]; // temp array which will hold string for a bit
    size_t i = 0;   // index variable

    size_t tempSize = sizeof(temp);
    strncpy(temp, strCom, tempSize - 1); // copy command into temp
    temp[tempSize - 1] = '\0';           // null terminator

    cmd->type = HEADING_COM;

    // skip command word
    while (temp[i] != ' ' && temp[i] != '\0')
    {
        i++;
    }

    // parse version number
    long version;
    int success = parsingHelp(temp, &i, &version, tempSize - 1);
    if (success != 0)
    {
        return -1; // error
    }
    cmd->version = version;

    // parse level
    long level;
    success = parsingHelp(temp, &i, &level, tempSize - 1);
    if (success != 0)
    {
        return -1; // error
    }

    // check if level between 1-3
    if (level < 1 || level > 3)
    {
        return -1; // error
    }

    cmd->level = level;

    // parse position
    long pos;
    success = parsingHelp(temp, &i, &pos, tempSize - 1);
    if (success != 0)
    {
        return -1; // error
    }
    cmd->pos = pos;

    return 0;
}

int parseLinkCom(const char *strCom, command *cmd)
{
    char temp[256]; // temp array which will hold string for a bit
    size_t i = 0;   // index variable

    size_t tempSize = sizeof(temp);
    strncpy(temp, strCom, tempSize - 1); // copy command into temp
    temp[tempSize - 1] = '\0';           // null terminator

    cmd->type = LINK_COM;

    // skip command word
    while (temp[i] != ' ' && temp[i] != '\0')
    {
        i++;
    }

    // parse version number
    long version;
    int success = parsingHelp(temp, &i, &version, tempSize - 1);
    if (success != 0)
    {
        return -1; // error
    }
    cmd->version = version;

    // parse start
    long start;
    success = parsingHelp(temp, &i, &start, tempSize - 1);
    if (success != 0)
    {
        return -1; // error
    }
    cmd->pos = start;

    // parse end
    long end;
    success = parsingHelp(temp, &i, &end, tempSize - 1);
    if (success != 0)
    {
        return -1; // error
    }
    cmd->end = end;

    // confirm that end is after start
    if (end <= start)
    {
        return -1;
    }

    // code to skip every space before url
    while (temp[i] == ' ')
    {
        i++;
    }

    // check url
    if (temp[i] == '\0')
    {
        return -1; // url missing
    }

    // now copy url
    size_t len = strlen(&temp[i]);
    cmd->content = malloc(len + 1); // +1 for null terminator
    if (cmd->content == NULL)
    {
        return -1; // memory not allocated
    }
    strcpy(cmd->content, &temp[i]);

    // check there is no newline
    for (size_t f = 0; cmd->content[f] != '\0'; f++)
    {
        if (cmd->content[f] == '\n')
        {
            free(cmd->content);
            return -1; // error if newline present
        }
    }

    return 0; // success!
}

int parseOnePosCom(const char *strCom, command *cmd, int type)
{

    // copy to a temp
    char temp[256];
    size_t i = 0;

    if (parseVersionPos(strCom, cmd, type, temp, sizeof(temp), &i) != 0)
    {
        return -1;
    }

    return 0; // success
}

int parseTwoPosCom(const char *strCom, command *cmd, int type)
{

    // copy to a temp
    char temp[256];
    strncpy(temp, strCom, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0'; // terminate at appropriate index according to size

    size_t i = 0;
    cmd->type = type;

    // skip command word and spacea after it to the relevant stuff later on
    while (temp[i] != ' ' && temp[i] != '\0')
    {
        i++;
    }

    // parse version
    long version;
    if (parsingHelp(temp, &i, &version, sizeof(temp) - 1) != 0)
    {
        return -1; // error found
    }
    cmd->version = version;

    long start; // starting of range
    if (parsingHelp(temp, &i, &start, sizeof(temp) - 1) != 0)
    {
        return -1;
    }
    cmd->pos = start; // starting value of range into pos

    long end; // ending of range
    if (parsingHelp(temp, &i, &end, sizeof(temp) - 1) != 0)
    {
        return -1;
    }
    cmd->end = end; // end of range

    // confirm that end is after start
    if (end <= start)
    {
        return -1;
    }

    return 0; // success!
}

// this func gets the command in string and converts it into my command struct
int parseCom(const char *strCom, command *cmd, int permissions)
{
    // checks
    if (strCom == NULL || cmd == NULL)
    {
        return -1; // error
    }

    // error if command too long (shouldnt be greater than 256 per spec)
    if (strlen(strCom) >= 256) // >= as strlen deosnt count null terminator
    {
        return -1;
    }

    // check for printable ascii chars which spec requires
    // spec says commands must be printable ascii (32-126) or newline
    for (size_t i = 0; strCom[i] != '\0'; i++) {
        // check if char is not printable and not a newline
        // learned about carriage return here so thought this might be trailing so should remove https://stackoverflow.com/questions/1552749/difference-between-cr-lf-lf-and-cr-line-break-types
        if ((strCom[i] < 32 || strCom[i] > 126) && strCom[i] != '\n' && strCom[i] != '\r') {
            return -1; // invalid character in command so error
        }
    }

    int success = 0; // variable to track if various functions were successful or not

    // initialize cmd
    memset(cmd, 0, sizeof(command)); // sets values in structure to 0

    // parse the command type string
    char comType[64];                          // 64 is big enough to hold the string so it works
    success = sscanf(strCom, "%63s", comType); // 63 means read up to 63 char max to leave space for null terminator
    if (success != 1)                          // should read a string for command. if it doesnt then error
    {
        return -1;
    }

    // parse all commands details now
    if (strcmp(comType, "INSERT") == 0)
    {
        if (permissions != WRITE)
        {
            return -1; // need to have write permission
        }
        return parseInsertCom(strCom, cmd);
    }
    else if (strcmp(comType, "HEADING") == 0)
    {
        if (permissions != WRITE)
        {
            return -1;
        }
        return parseHeadingCom(strCom, cmd);
    }
    else if (strcmp(comType, "DEL") == 0)
    {
        if (permissions != WRITE)
        {
            return -1;
        }
        return parseDelCom(strCom, cmd);
    }
    else if (strcmp(comType, "LINK") == 0)
    {
        if (permissions != WRITE)
        {
            return -1;
        }
        return parseLinkCom(strCom, cmd);
    }
    else if (strcmp(comType, "BLOCKQUOTE") == 0)
    {
        if (permissions != WRITE)
        {
            return -1;
        }
        return parseOnePosCom(strCom, cmd, BLOCKQUOTE_COM);
    }
    else if (strcmp(comType, "NEWLINE") == 0)
    {
        if (permissions != WRITE)
        {
            return -1;
        }
        return parseOnePosCom(strCom, cmd, NEWLINE_COM);
    }
    else if (strcmp(comType, "ORDERED_LIST") == 0)
    {
        if (permissions != WRITE)
        {
            return -1;
        }
        return parseOnePosCom(strCom, cmd, ORDEREDLIST_COM);
    }
    else if (strcmp(comType, "UNORDERED_LIST") == 0)
    {
        if (permissions != WRITE)
        {
            return -1;
        }
        return parseOnePosCom(strCom, cmd, UNORDEREDLIST_COM);
    }
    else if (strcmp(comType, "HORIZONTAL_RULE") == 0)
    {
        if (permissions != WRITE)
        {
            return -1;
        }
        return parseOnePosCom(strCom, cmd, HORIZONTALRULE_COM);
    }
    else if (strcmp(comType, "BOLD") == 0)
    {
        if (permissions != WRITE)
        {
            return -1;
        }
        return parseTwoPosCom(strCom, cmd, BOLD_COM);
    }
    else if (strcmp(comType, "ITALIC") == 0)
    {
        if (permissions != WRITE)
        {
            return -1;
        }
        return parseTwoPosCom(strCom, cmd, ITALIC_COM);
    }
    else if (strcmp(comType, "CODE") == 0)
    {
        if (permissions != WRITE)
        {
            return -1;
        }
        return parseTwoPosCom(strCom, cmd, CODE_COM);
    }
    else if (strcmp(comType, "DOC?") == 0)
    {
        cmd->type = DOC_COM;
        return 0;
    }
    else if (strcmp(comType, "PERM?") == 0)
    {
        cmd->type = PERM_COM;
        return 0;
    }
    else if (strcmp(comType, "LOG?") == 0)
    {
        cmd->type = LOG_COM;
        return 0;
    }
    else if (strcmp(comType, "DISCONNECT") == 0)
    {
        cmd->type = DISCONNECT_COM;
        return 0;
    }

    return -1; // if we got here then command type is invalid
}

// ================================= EXECUTION STUFF =================================

// this func creates a new log and returns its pointer
commandlog *initializeLog(void)
{
    commandlog *log = malloc(sizeof(commandlog)); // allocate mem
    if (log == NULL)                              // check if mem allocation worked
    {
        return NULL;
    }

    // initialize values
    log->count = 0;

    // set all mem to 0 to initialize everything
    memset(log->entries, 0, sizeof(log->entries));

    return log;
}

// this funct adds command result to our log
void logCom(commandlog *log, const char *response, uint64_t version)
{
    // basic check taht we have log and within range
    if (log == NULL || log->count >= MAX_LOG_ENTRIES)
    {
        return; // error
    }

    int i = log->count; // index variable for wehere to add

    // allocate memory for response string
    log->entries[i].responseStr = malloc(strlen(response) + 1); // +1 for null terminator
    if (log->entries[i].responseStr == NULL)
    {
        return; // error memory not allocated propertly
    }

    // now actually copy data
    strcpy(log->entries[i].responseStr, response);
    log->entries[i].version = version;
    log->count++; // increase the count
}

// this function gets a report in correct format per spec of all log entries for LOG? command
char *getLogReport(commandlog *log)
{
    // special case if log empty
    if (log == NULL || log->count == 0)
    {
        char *emptyReport = malloc(32); // "LOG?" + "\n" + "No commands executed yet" + "\n" = 31 chars so allocate for 32 for null terminator
        if (emptyReport == NULL)
        {
            return NULL; // error mem not allocated
        }
        // otherwise
        snprintf(emptyReport, 32, "LOG?\nNo commands executed yet\n");
        return emptyReport;
    }

    // now if not empty

    // ifFirst calculate the amount of memory we need
    size_t total = 5; // "LOG\n"
    int i = 0;
    for (i = 0; i < log->count; i++)
    {
        /// add version, edit, response, newline
        total = total + 30 + strlen(log->entries[i].responseStr); // 30 should be enough for other stuff to be safe
    }

    // allocate mem
    char *report = malloc(total);
    if (report == NULL) // check allocation
    {
        char *errorReport = malloc(30); // if failed need to return error report so allocate memory for that
        if (errorReport == NULL)        // check allocation
        {
            return NULL;
        }
        snprintf(errorReport, 30, "LOG?\nError generating log\n"); // insert the message
        return errorReport;
    }

    // initialize report as empty
    report[0] = '\0';
    // pos makes sure we dont write past end of allocated mem
    int pos = 0;
    // using += updates pos to point to end of string after this
    pos += snprintf(report + pos, total - pos, "LOG?\n"); // add LOG? header to start of report string

    uint64_t currentVersion = 0;
    int ifFirst = 1; // to track the First log entrty (1 true 0 false)

    // now go through all the log entries
    for (i = 0; i < log->count; i++)
    {
        // check if we need to start a new version area
        if (ifFirst == 1 || log->entries[i].version != currentVersion)
        {
            if (ifFirst != 1) // if not First
            {
                pos += snprintf(report + pos, total - pos, "END\n"); // same concept as before
            }
            ifFirst = 0;                              // now we seen First entry so false now
            currentVersion = log->entries[i].version; // update currentvers to present value

            // version header for report
            pos += snprintf(report + pos, total - pos, "VERSION %lu\n", (unsigned long)currentVersion); // again same logic as earlier
        }

        // add this commands result

        // below report + pos again means start writing at current position. total - pos telling snprintf max bytes to write.
        pos += snprintf(report + pos, total - pos, "EDIT %s\n", log->entries[i].responseStr); // start with EDIT keyword, then add actual response string, then newline
    }

    // close the last one
    pos += snprintf(report + pos, total - pos, "END\n"); // same logic pretty much as before
    return report;
}

// cleanup fucntion
void freeLog(commandlog *log)
{
    if (log == NULL)
    {
        return; // nothing to free
    }

    int i = 0;
    for (i = 0; i < log->count; i++)
    {
        free(log->entries[i].responseStr); // free each string
    }
    free(log); // free the log
}

// this function executes given command on doc
char *executeCom(command *cmd, document *doc, int permissions)
{
    // basic checks and handling per spec
    // check for null inputs
    if (cmd == NULL || doc == NULL)
    {
        char *resp = malloc(30);
        if (resp == NULL) // check if allocated
        {
            return NULL;
        }
        strcpy(resp, "Reject INVALID_COMMAND"); // return error resposse
        return resp;
    }

    // check if command needs permissions
    if (permissions != WRITE && cmd->type <= LINK_COM) // everything below link com needs write permission
    {
        char *resp = malloc(30);
        if (resp == NULL) // check if allocated
        {
            return NULL;
        }
        strcpy(resp, "Reject UNAUTHORISED"); // error response
        return resp;
    }

    int success = 0;       // to check if functions are successful or not
    char *response = NULL; // for response output

    // now switch commands based on command type
    switch (cmd->type)
    {
    case INSERT_COM:
        if (cmd->content != NULL) // if there is content do this version
        {
            success = markdown_insert(doc, cmd->version, cmd->pos, cmd->content);
        }
        else
        {
            success = markdown_insert(doc, cmd->version, cmd->pos, "");
        }
        break;
    case DELETE_COM:
        success = markdown_delete(doc, cmd->version, cmd->pos, cmd->end);
        break;
    case NEWLINE_COM:
        success = markdown_newline(doc, cmd->version, cmd->pos);
        break;
    case HEADING_COM:
        success = markdown_heading(doc, cmd->version, cmd->level, cmd->pos);
        break;
    case BOLD_COM:
        success = markdown_bold(doc, cmd->version, cmd->pos, cmd->end);
        break;
    case ITALIC_COM:
        success = markdown_italic(doc, cmd->version, cmd->pos, cmd->end);
        break;
    case BLOCKQUOTE_COM:
        success = markdown_blockquote(doc, cmd->version, cmd->pos);
        break;
    case ORDEREDLIST_COM:
        success = markdown_ordered_list(doc, cmd->version, cmd->pos);
        break;
    case UNORDEREDLIST_COM:
        success = markdown_unordered_list(doc, cmd->version, cmd->pos);
        break;
    case CODE_COM:
        success = markdown_code(doc, cmd->version, cmd->pos, cmd->end);
        break;
    case HORIZONTALRULE_COM:
        success = markdown_horizontal_rule(doc, cmd->version, cmd->pos);
        break;
    case LINK_COM:
        success = markdown_link(doc, cmd->version, cmd->pos, cmd->end, cmd->content);
        break;
    case DOC_COM:
    {
        // get doc content
        char *content = markdown_flatten(doc);
        if (content != NULL)
        {
            response = malloc(10 + strlen(content)); // allocating more than enough memory
            if (response != NULL)
            {
                snprintf(response, 10 + strlen(content), "DOC?\n%s", content);
            }
            free(content); // clean
            return response;
        }
        else
        {
            response = malloc(40);
            if (response == NULL)
            {
                return NULL; // error
            }
            snprintf(response, 40, "DOC?\nError flattening document");
            return response;
        }
    }
    case PERM_COM:
    {
        // get permission as string
        char *permStr = "unknown";
        if (permissions == WRITE)
        {
            permStr = "write";
        }
        else if (permissions == READ)
        {
            permStr = "read";
        }

        response = malloc(strlen(permStr) + 10); // allocate memory
        if (response == NULL)
        {
            return NULL; // error
        }
        snprintf(response, strlen(permStr) + 10, "PERM?\n%s", permStr); // update resposne
        return response;
    }
    case LOG_COM:
        // needs to be handled by caller with log access
        return NULL;
    case DISCONNECT_COM: // disconnects
        return NULL; // per spec shouldnt send a message
    default: // error case invalid command
        response = malloc(30);
        if (response != NULL)
        {
            snprintf(response, 30, "Reject INVALID_COMMAND");
        }
        return response;
    }

    // deal with the success variables from all the markdown functions
    // 0 is sucess, -1 is nvalid cursor pos, -2 is deleted pos, -3 is outdated version, else unknown error
    if (success == 0)
    {
        response = malloc(8);
        if (response != NULL)
        {
            snprintf(response, 8, "SUCCESS");
        }
    }
    else if (success == -1)
    {
        response = malloc(25);
        if (response != NULL)
        {
            snprintf(response, 25, "Reject INVALID_POSITION");
        }
    }
    else if (success == -2)
    {
        response = malloc(25);
        if (response != NULL)
        {
            snprintf(response, 25, "Reject DELETED_POSITION");
        }
    }
    else if (success == -3)
    {
        response = malloc(25);
        if (response != NULL)
        {
            snprintf(response, 25, "Reject OUTDATED_VERSION");
        }
    }
    else
    {
        response = malloc(22);
        if (response != NULL)
        {
            snprintf(response, 22, "Reject UNKNOWN_ERROR");
        }
    }

    return response;
}