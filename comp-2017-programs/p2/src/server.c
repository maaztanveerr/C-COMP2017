#define _POSIX_C_SOURCE 200809L // ed thread 2219 helped me with this
#include "server.h"

// array to track all clients
clientinfo clients[MAX_CLIENTS]; // capped at our max
int clientcount = 0;             // current count of clients

// log to track all commands for LOG? command
commandlog *serverlog = NULL; // currently null at start

// queue to store commands while another command is being processed
commandQ cmdq;

document *shareddoc = NULL; // create a shared document globally

// ================================= FUNCTIONS FOR COMMAND =================================

// function proccesses all commands in queue
void qcommandprocessor(void)
{
    // basic check
    if (cmdq.count == 0)
    { // if q empty, nothing to do
        return;
    }

    char *responses[MAX_QCOMMANDS];
    int resCount = 0;

    // now process all command
    for (int i = 0; i < cmdq.count; i++)
    {
        command *cmd = &cmdq.commands[i]; // move a command from q into cmd for processing

        // execute command on shared doc
        char *resp = executeCom(cmd, shareddoc, WRITE); // i already added checks for read only clients in my parseCom function
                                                        // which means that any command that already reached the q is okay so i can give the server full write access

        // skip special command cases which client handler will handle
        if (cmd->type != DOC_COM && cmd->type != PERM_COM && cmd->type != LOG_COM)
        {
            // adding command response to log
            logCom(serverlog, resp, shareddoc->version);
            // save response for broadcast
            responses[resCount++] = resp;
        }
        else
        {
            free(resp);
        }
    }

    // if we got responses, time to increment version and broadcast update
    if (resCount > 0) // basic check
    {
        markdown_increment_version(shareddoc);                 // update doc version
        broadcasting(shareddoc->version, responses, resCount); // broadcast
    }

    // free response now cleaning up
    for (int i = 0; i < resCount; i++)
    {
        free(responses[i]);
    }

    // clean queue now bcs processing is done
    for (int i = 0; i < cmdq.count; i++)
    {
        /// adding a check here to make sure we dont free anything null
        if (cmdq.commands[i].content != NULL)
        {
            free(cmdq.commands[i].content);
        }
        free(cmdq.commandstr[i]);
    }
    cmdq.count = 0; // q empty
}

// ================================= CLIENT MANAGEMENT AND SIGNAL HANDLER=================================

// add client to track
void addClient(pid_t pid, int writingfd, int readingfd, int permissions)
{
    // basic check to see if there is enough space for an additional client
    if (clientcount < MAX_CLIENTS)
    { // if space, set the appropriate values
        clients[clientcount].pid = pid;
        clients[clientcount].writingfd = writingfd;
        clients[clientcount].readingfd = readingfd;
        clients[clientcount].permissions = permissions;
        clients[clientcount].connected = true;
        clientcount++; // one more client now
    }
    // if no space, function does nothing
}

// disconnect a client
void disconnectClient(pid_t pid)
{
    printf("Disconnecting client with PID: %d\n", pid);

    // looking for client with given pid
    for (int i = 0; i < clientcount; i++)
    {
        if (clients[i].pid == pid) // if found
        {
            if (clients[i].connected)
            {
                // clean up and disconnect
                close(clients[i].readingfd);
                close(clients[i].writingfd);

                // unlinking fifos
                char servToclient[128];
                char clientToserv[128];
                snprintf(clientToserv, sizeof(clientToserv), "FIFO_C2S_%d", pid);
                snprintf(servToclient, sizeof(servToclient), "FIFO_S2C_%d", pid); // these are the names of the fifos that i created earlier and i need to unlink them

                unlink(clientToserv);
                unlink(servToclient);

                clients[i].connected = false; // disconnect here

                // ensuring doc is saved after disconnecting
                FILE *save = fopen("doc.md", "w");
                if (save != NULL)
                {
                    markdown_print(shareddoc, save);
                    fclose(save);
                    printf("Document saved to doc.md\n");
                }
            }
            printf("Client %d disconnected and FIFOs unlinked\n", pid);
            return;
        }
    }
}

// function broadcast updates and changes to version acroass all of the connected clients
void broadcasting(uint64_t version, char **responses, int resCount)
{ // responses is the result from the execution function for commands. its an array of strings. also, rescount is the num of responses in array
    // need to calculate memory needed for broadcast
    // "VERSION " = 8 chars, version number could vary so im using 15 for good measure, null terminator is 1, end/n is 4 so total is 28. i'll use 32 to be safe
    size_t totalsize = 32;

    // go thru responses to add their length to our size
    for (int i = 0; i < resCount; i++)
    {
        // adding "EDIT " (5) + length of response + newline
        totalsize = totalsize + 6 + strlen(responses[i]);
    }

    char *broadcast = malloc(totalsize); // allocate memory
    // basic check
    if (broadcast == NULL)
    {
        return; // error memory not allocated
    }

    size_t pos = 0; // used to keep track of position in broadcast

    // writing now:

    // version
    //  adding pos to the following snpritnf so that we update our position after this so next write starts after what we just wrote
    pos = pos + snprintf(broadcast + pos, totalsize - pos, "VERSION %lu\n", (unsigned long)version); // ive already used this technique in several places in commands.c

    // responses
    // now go through responses to add to broadcast
    for (int i = 0; i < resCount; i++)
    {
        pos = pos + snprintf(broadcast + pos, totalsize - pos, "EDIT %s\n", responses[i]); // same logic but for the responses instead of version now
    }

    // now for end
    pos = pos + snprintf(broadcast + pos, totalsize - pos, "END\n"); // same logic

    // now since our braodcast is ready, send it to clients
    for (int i = 0; i < clientcount; i++)
    {
        // only need to send to connected clients
        if (clients[i].connected == true)
        {
            // use write and the clients writing fd to send broadcast
            ssize_t status = write(clients[i].writingfd, broadcast, strlen(broadcast));

            // check if worked
            if (status < 0)
            {
                clients[i].connected = false; // meaning disconnect and skip as write to them isnt possible
            }
        }
        // if no clients are connceted then function ends
    }
}

void clean(int read, int write)
{
    close(read);
    close(write);
    pthread_exit(NULL);
}

void *clienthandler(void *arg)
{
    pid_t client_pid = *(pid_t *)arg; // basically means that im telling c that arg is a pointer to a pid so cast it as such and then dereference it to get the value
    free(arg);                        // copied value so dont need this memory for arg //free

    // for fifo from client to server and server to client
    char servToclient[128];
    char clientToserv[128]; // 128 is an approximate i used because it should be big enough for my purposes

    // from spec: • FIFO_C2S_<client_pid> (Client-to-Server)
    // • FIFO_S2C_<client_pid> (Server-to-Client)

    snprintf(clientToserv, sizeof(clientToserv), "FIFO_C2S_%d", client_pid);
    snprintf(servToclient, sizeof(servToclient), "FIFO_S2C_%d", client_pid);

    unlink(servToclient);
    unlink(clientToserv); // delete any pre existing file with chosen names

    // mkfifo to build a named pipe for client and server
    int success = mkfifo(clientToserv, 0666);
    if (success == -1) // check if worked
    {
        perror("MKFIFO has failed(clienttoserv)");
        pthread_exit(NULL);
    }

    success = 0;
    success = mkfifo(servToclient, 0666);
    if (success == -1) // check if worked
    {
        perror("MKFIFO has failed (servtoclient)");
        unlink(clientToserv); // remove the previous one if that was a success and this wasnt
        pthread_exit(NULL);
    }

    printf("FIFO files were created.\n");

    success = 0;
    success = kill(client_pid, SIGRTMIN + 1); // let the client know that fifo have been created and it can continue
    if (success == -1)                        // check
    {
        perror("Couldnt sent SIGRTMIN +1 to client");
    }
    else
    {
        printf("Server sent sigrtmin+1 to client\n");
    }

    // opening the fifos

    // open for write
    int writing = open(servToclient, O_WRONLY);
    if (writing == -1)
    {
        perror("Failed to open server to client file for writing");
        pthread_exit(NULL);
    }
    // open for read
    int reading = open(clientToserv, O_RDONLY);
    if (reading == -1)
    {
        perror("Failed to open client to server for reading");
        close(writing); // cleaning up prior resouces for writing
        pthread_exit(NULL);
    }

    // read username
    char username[256];
    ssize_t amountRead = read(reading, username, sizeof(username) - 1); // -1 to leave space for null terminator
    if (amountRead <= 0)                                                // check if error or at file end
    {
        perror("Server could not read client's username");
        clean(reading, writing); // clean up resouces
    }

    username[amountRead] = '\0'; // add null terminate at end

    FILE *rolesfile = fopen("roles.txt", "r"); // open the roles file
    if (rolesfile == NULL)                     // check if properly opened
    {
        perror("Couldnt open roles.txt");
        clean(reading, writing); // clean up
    }

    char user[128];    // username from roles file
    char role[128];    // the role
    char linenum[512]; // number of lines in the file

    char outputRole[128]; // the role to be output to client
    bool found = false;

    while (fgets(linenum, sizeof(linenum), rolesfile) != NULL)
    {                                                        // do the following until EOF
        if (sscanf(linenum, "%127s %127s", user, role) == 2) // used these resources to learn more about sscanf https://stackoverflow.com/questions/27054634/how-is-sscanf-different-from-scanf-in-the-following-statements
                                                            // https://stackoverflow.com/questions/5688258/parsing-a-line-in-c
        {
            if (strcmp(user, username) == 0)
            {
                strcpy(outputRole, role); // copy the associated role into our local array
                found = true;
                break;
            }
        }
    }

    fclose(rolesfile); // close the file

    success = 0;
    // if username not found in roles.txt, reject.
    if (found == false)
    {
        success = write(writing, "Reject UNAUTHORISED\n", strlen("Reject UNAUTHORISED\n"));
        if (success < 0)
        {
            printf("Error in writing");
            clean(reading, writing);
        }
        sleep(1);

        // clean up resouces
        unlink(clientToserv);
        unlink(servToclient);
        clean(reading, writing);
        return NULL;
    }

    // if found, send a response to the client which has its role
    char response[256];
    snprintf(response, sizeof(response), "%s\n", outputRole);          // writing the role to be output in the correct format to resposnse
    ssize_t byteswritten = write(writing, response, strlen(response)); // write to shared file
    if (byteswritten < 0)
    {
        printf("Error in writing");
        clean(reading, writing);
    }
    fsync(writing);
    markdown_increment_version(shareddoc); //ensures deferred edits are applied before sending to new client

    // flatten document
    char *content = markdown_flatten(shareddoc);
    // doc length in bytes
    size_t doclen = 0;
    if (content == NULL)
    {
        content = malloc(1);
        if (content == NULL)
        {
            perror("FAILED MEMORY ALLOCATION");
            clean(reading, writing);
        }
        content[0] = '\0';
    }
    else
    {
        doclen = strlen(content);
    }

    // Version number.
    char version[32];
    snprintf(version, sizeof(version), "%lu\n", shareddoc->version);
    byteswritten = write(writing, version, strlen(version));
    if (byteswritten < 0)
    {
        printf("Error in writing");
        clean(reading, writing);
    }
    fsync(writing);

    char length[32];
    snprintf(length, sizeof(length), "%zu\n", doclen);
    byteswritten = write(writing, length, strlen(length));
    if (success < 0)
    {
        printf("Error in writing len");
        free(content);
        clean(reading, writing);
    }
    fsync(writing);

    // send the actual content now
    byteswritten = write(writing, content, doclen);
    if (byteswritten < 0)
    {
        printf("Error in writing");
        free(content);
        clean(reading, writing);
    }
    fsync(writing);

    // now find permission level from roles
    int permissions = 0; // initialize with no permisssion

    if (strcmp(outputRole, "write") == 0)
    {
        permissions = WRITE;
    }
    else if (strcmp(outputRole, "read") == 0)
    {
        permissions = READ;
    }

    // add the client for tracking
    addClient(client_pid, writing, reading, permissions);

    // now enter the command loop to handle the input
    char commandinput[512]; // array to read commands. 512 should be big enough
    FILE *clientinput = fdopen(reading, "r");
    if (clientinput == NULL)
    {
        perror("couldnt open clients input in server");
        // clean up
        disconnectClient(client_pid);
        return NULL;
    }

    while (fgets(commandinput, sizeof(commandinput), clientinput))
    {
        /// stripping trailing characters
        size_t len = strlen(commandinput);
        // learned about carriage return here so thought this might be trailing so should remove https://stackoverflow.com/questions/1552749/difference-between-cr-lf-lf-and-cr-line-break-types
        while (len > 0 && (commandinput[len - 1] == '\n' || commandinput[len - 1] == '\r'))
        {
            commandinput[--len] = '\0';
        }

        // parse command now
        command currcmd;
        if (parseCom(commandinput, &currcmd, permissions) != 0)
        {
            // invalid so skip this one
            continue; // move on to next iteration
        }

        // handle special commands immediately
        if (currcmd.type == DISCONNECT_COM)
        {
            // client wants to disconnect
            printf("Client %d disconnecting\n", client_pid);

            qcommandprocessor(); // process pending edits

            // Update version if changes were applied
            if (shareddoc->changeList != NULL)
            {
                markdown_increment_version(shareddoc);
            }

            FILE *save = fopen("doc.md", "w");
            if (save != NULL)
            {
                // if there are no problems then
                markdown_print(shareddoc, save);
                fclose(save);
            }

            // disconnect and cleanup
            disconnectClient(client_pid);
            fclose(clientinput);
            free(content);
            pthread_exit(NULL);
            return NULL;
        }
        else if (currcmd.type == DOC_COM)
        {
            // client wants current document
            char *response = executeCom(&currcmd, shareddoc, permissions);
            if (response != NULL)
            { // if no error then write and free
                write(writing, response, strlen(response));
                free(response);
            }
        }
        else if (currcmd.type == PERM_COM)
        {
            // client wants to know its permissions
            char *response = executeCom(&currcmd, shareddoc, permissions);
            if (response != NULL)
            {
                write(writing, response, strlen(response));
                free(response);
            }
        }
        else if (currcmd.type == LOG_COM)
        {
            // client wants command log
            char *logreport = getLogReport(serverlog);
            if (logreport != NULL)
            {
                write(writing, logreport, strlen(logreport));
                free(logreport);
            }
        }
        else
        {
            // regular commands now so add to the queue
            if (cmdq.count < MAX_QCOMMANDS) // if space present
            {
                // get command into q
                memcpy(&cmdq.commands[cmdq.count], &currcmd, sizeof(command));

                // save the command string
                // first allocate memory
                cmdq.commandstr[cmdq.count] = malloc(strlen(commandinput) + 1); // +1 for null terminator
                if (cmdq.commandstr[cmdq.count] != NULL)
                {
                    strcpy(cmdq.commandstr[cmdq.count], commandinput);
                }

                // increase queue count
                cmdq.count++;
            }
        }
    }

    // cleanup
    disconnectClient(client_pid); // if we got to here then client disconnected or some error happened
    fclose(clientinput);
    free(content);      // as flatten allocates memory, need to free here
    pthread_exit(NULL); // exit the thread
    return NULL;
}

void signalHandler(int signal, siginfo_t *info, void *status)
{
    (void)status; // not using it
    if (signal == SIGRTMIN)
    {                                             // check if sigrtmin is the signal
        pid_t *clientPID = malloc(sizeof(pid_t)); // and then allocate memory to ensure that pid is stored as long as we need it.
        if (clientPID == NULL)
        {
            // malloc failed
            return;
        }
        *clientPID = info->si_pid;
        ;

        pthread_t threadID;
        int success = pthread_create(&threadID, NULL, clienthandler, clientPID); // create a thread utilizing clienthandler
        if (success != 0)
        {
            perror("thread creation faileed");
            free(clientPID);
            return;
        }

        pthread_detach(threadID); // clean up resuoruces
    }
}

// ================================= MAIN =================================

int main(int argc, char *argv[])
{

    shareddoc = markdown_init(); // initialize document
    serverlog = initializeLog(); // initialize log
    cmdq.count = 0;              // initialize count

    // our epoll instance to check for quit command
    int epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        // epoll failed soerror
        perror("couldnt create epoll");
        return 1;
    }

    // setting up the struct now
    struct epoll_event ep;
    ep.events = EPOLLIN;       // basically means tell me when there is data available to read from user
    ep.data.fd = STDIN_FILENO; // storing descriptor we want to look out for from standard input

    // registering stdin with our epoll basically saying that we are checking this specific fd for read events
    // epoll_ctl_add is the operation to add new fd to look at     
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &ep) == -1)
    {
        // error so clean up and return
        perror("epoll ctl failed");
        close(epollfd);
        return 1;
    }

    // basic check to see if the time interval is given
    if (argc != 2)
    {
        printf("Arguments are not correct.");
        return 1;
    }

    // converting string time interval from arg to int
    char *last;
    long time = strtol(argv[1], &last, 10); 

    // check if conversion was okay and the input was fine
    if (*last != '\0' || time <= 0)
    {
        printf("Entered time interval is invalid: %s\n", argv[1]);
        return 1;
    }

    // if everything is fine, output the pid to stdout for client to connect
    printf("Server PID: %d\n", getpid());

    // set up signal in main
    struct sigaction action;
    memset(&action, 0, sizeof(action)); // initialize
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = signalHandler;
    sigemptyset(&action.sa_mask); // dont block

    int success = sigaction(SIGRTMIN, &action, NULL);
    if (success == -1)
    {
        perror("sigaction error");
        return 1;
    }

    // convert time interval to a number
    long timeInterval = strtol(argv[1], NULL, 10);
    if (timeInterval <= 0)
    {
        printf("time interval must be positive\n");
        return 1;
    }

    // now main loop with processing at specific intervals
    while (1)
    {
        // sleep as required
        // for some reason usleep wasnt working so i learned about this alternative nanosleep here https://stackoverflow.com/questions/1157209/is-there-an-alternative-sleep-function-in-c-to-milliseconds
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = timeInterval * 1000000; // convert milliseconds to nanoseconds
        nanosleep(&ts, NULL);

        // process commands in q
        qcommandprocessor();

        // check for quit without blocking
        // creating an array to hold events that epollwait will return
        struct epoll_event events[1]; // 1 bcs we are only looking at 1 fd (stdin)

        // using epoll wait to check if stdin has data available
        int data = epoll_wait(epollfd, events, 1, 0);

        if (data > 0) // means data is available
        {
            char reader[64]; // array to store data. 64 should be enough

            // read using fgets
            if (fgets(reader, sizeof(reader), stdin) != NULL)
            {
                // we dont want newlines so replace with null terminator
                int i = 0;
                while (reader[i] != '\0')
                {
                    if (reader[i] == '\n')
                    {
                        reader[i] = '\0';
                        break;
                    }
                    i++;
                }

                // check if input is quit
                if (strcmp(reader, "QUIT") == 0)
                {
                    // check if clinets connected. cant quit if there are clients
                    int connections = 0;
                    for (int i = 0; i < clientcount; i++)
                    {
                        if (clients[i].connected == true)
                        {
                            connections++; // if there are clients connected then increment
                        }
                    }

                    // if connected, dont quit reject instead
                    if (connections > 0)
                    {
                        printf("QUIT rejected, %d clients still connected.\n", connections);
                    }
                    else
                    {
                        // no one connected so quit now
                        printf("QUIT command recieved, shutting down...\n");

                        qcommandprocessor();
                        // ensure any pending changes are applied before saving
                        if (shareddoc->changeList != NULL)
                        {
                            markdown_increment_version(shareddoc);
                        }

                        // save document to doc.md
                        FILE *save = fopen("doc.md", "w");
                        if (save != NULL)
                        {
                            // if there are no problems then
                            markdown_print(shareddoc, save);
                            fclose(save);
                        }

                        // clean up time
                        markdown_free(shareddoc);
                        freeLog(serverlog);
                        close(epollfd);
                        exit(0);
                    }
                }
            }
        }
        else if (data < 0)
        {
            perror("epoll wait failure");
            break; // error
        }
    }

    close(epollfd); // clean up (if we manage to get here)
}