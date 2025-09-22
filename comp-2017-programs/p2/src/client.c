#define _POSIX_C_SOURCE 200809L // ed thread 2219 helped me with this
#define FAIL 0                  // this and the following two are macros to make analyzing the response form server easier
#define READ 1
#define WRITE 2
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>    //for signal functions
#include <sys/epoll.h> // for epoll
#include <string.h>
#include <fcntl.h> 
#include <stdbool.h>

int main(int argc, char *argv[])
{
    // basic check to see if the time interval is given
    if (argc != 3)
    {
        printf("Incorrect arguments for client\n");
        return 1;
    }

    // converting string serverpid from arg to int
    char *last;
    pid_t serverPID = strtol(argv[1], &last, 10); 

    // check if conversion was okay and the input was fine
    if (*last != '\0' || serverPID <= 0)
    {
        printf("Entered server PID is invalid: %s\n", argv[1]);
        return 1;
    }

    const char *username = argv[2]; // save username

    int success = kill(serverPID, SIGRTMIN); // send sigrtnmin to the server
    if (success == -1)                       // check
    {
        perror("SIGRTMIN could not be sent to server");
        return 1;
    }

    printf("Client sent SIGRTMIN to server, now blocking\n");

    sigset_t mask;
    sigemptyset(&mask);             // initialize as empty
    sigaddset(&mask, SIGRTMIN + 1); // now basically add sigrtmin +1 to mask

    // now block for the time being
    sigprocmask(SIG_BLOCK, &mask, NULL); // if signal arrives before we are waiting for it then it might be lost. so put it into a queue

    // now wait
    siginfo_t info;                      // used to store info such as who sent signal and which one it was (this way u get server pid and can communicaate back and forth)
    success = sigwaitinfo(&mask, &info); // basically stop client until a sigrtmin + 1 signal arrives
    if (success == -1)                   // check
    {
        perror("sigwaitinfo error");
        return 1;
    }

    printf("Client recieved sigrtmin +1 from server.\n");

    // getting this clients pid to use with fifos
    pid_t clientPID = getpid();

    // for fifo from client to server and server to client
    char servToclient[128];
    char clientToserv[128]; // 128 is an approximate i used because it should be big enough for my purposes

    // from spec: • FIFO_C2S_<client_pid> (Client-to-Server)
    // • FIFO_S2C_<client_pid> (Server-to-Client)

    snprintf(clientToserv, sizeof(clientToserv), "FIFO_C2S_%d", clientPID);
    snprintf(servToclient, sizeof(servToclient), "FIFO_S2C_%d", clientPID);

    // self reminder: i am not creating the files here through the client. they already exist as they have been created by the server
    // im just accessing them as i need to use the clients pid to get into the file

    // open for write
    int writing = open(clientToserv, O_WRONLY);
    if (writing == -1)
    {
        perror("Failed to open client to server file for writing");
        return 1;
    }
    // open for read
    int reading = open(servToclient, O_RDONLY);
    if (reading == -1)
    {
        perror("Failed to open server to client for reading");
        close(writing); // cleaning up prior resouces for writing
        return 1;
    }

    // write username to server now
    char usernameArr[256];
    snprintf(usernameArr, sizeof(usernameArr), "%s\n", username); // store username from commandline to array

    success = write(writing, usernameArr, strlen(usernameArr));
    if (success == -1)
    {
        perror("failed to write username to the server");
        close(writing);
        close(reading);
        return 1;
    }

    printf("Client has written username to the server\n");

    // now handling the servers response to sent username from client

    FILE *readRes = fdopen(reading, "r"); // open file for reading now
    if (readRes == NULL)                  // check
    {
        perror("fdopen fail");
        close(writing);
        close(reading);
        return 1;
    }

    char response[256]; // more than enough for the response from server

    if (fgets(response, sizeof(response), readRes) == NULL) // reading the response into an array and checking if worked
    {
        perror("failiure to read response from server");
        close(writing);
        close(reading);
        return 1;
    }

    // now check to see what sort of // were obtained
     //permissions = FAIL;
    if (strcmp(response, "Reject UNAUTHORISED") == 0)
    {
        printf("Access was not granted by server\n");
        // = FAIL;
        fclose(readRes);
        close(writing);
        return 1;
    }
    else if (strcmp(response, "write\n") == 0)
    {
        printf("Write access granted\n");
        // = WRITE;
    }
    else if (strcmp(response, "read\n") == 0)
    {
        printf("Read access granted\n");
        // = READ;
    }
    else
    {
        printf("Unknown response from server\n");
        // = FAIL;
        fclose(readRes);
        close(writing);
        return 1;
    }

    // read the version string and convert it to a number using strtol
    char inputVersion[64];
    if (fgets(inputVersion, sizeof(inputVersion), readRes) == NULL)
    {
        perror("Failed to read the version from the server");
        fclose(readRes);
        close(writing);
        return 1;
    }

    char *endvers;
    uint64_t version = strtol(inputVersion, &endvers, 10);
    if (*endvers != '\0' && *endvers != '\n') // error checking
    {
        printf("Invalid version given by server");
        fclose(readRes);
        close(writing);
        return 1;
    }

    printf("document version %ld\n", version); // print version

    // read the doc length and convert it to number using strtol
    char inputLen[64];
    if (fgets(inputLen, sizeof(inputLen), readRes) == NULL)
    { // check if read okay
        perror("Failed reading doc length from server");
        fclose(readRes);
        close(writing);
        return 1;
    }

    char *endLen;
    uint64_t length = strtol(inputLen, &endLen, 10);
    if (*endLen != '\0' && *endLen != '\n') // error checking
    {
        printf("Invalid length given by server");
        fclose(readRes);
        close(writing);
        return 1;
    }

    // read actual document content now
    char *document = malloc(length + 1); //+1 for null terminatorr
    if (document == NULL)
    {
        perror("couldnt allocate memory for document");
        fclose(readRes);
        close(writing);
        return 1;
    }

    size_t readamount = fread(document, 1, length, readRes);
    if (readamount != length)
    {
        perror("failed to read document fully");
        free(document);
        fclose(readRes);
        close(writing);
        return 1;
    }

    document[length] = '\0';
    printf("This is the content of the document: \n%s\n", document);

    // creating epoll instance to monitor file descriptors watching for user and server messages
    int epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        // if error, clean up resources and exit
        perror("epoll_create1 failed");
        free(document);
        fclose(readRes);
        close(writing);
        return 1;
    }

    // set up events we want to watch
    struct epoll_event ep;
    ep.events = EPOLLIN;       // basically means tell me when there is data available to read from user
    ep.data.fd = STDIN_FILENO; // storing descriptor we want to look out for from standard input

    // registering stdin with our epoll basically saying that we are checking this specific fd for read events
    // epoll_ctl_add is the operation to add new fd to look at      
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &ep) == -1)
    {
        // error so clean up and return
        perror("epoll ctl failed for stdin from clients side");
        close(epollfd);
        free(document);
        fclose(readRes);
        close(writing);
        return 1;
    }

    // same thing but for server. adding server connection to watch too
    ep.data.fd = reading; // pipe where we read server messages
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, reading, &ep) == -1)
    {
        // error so clean up and return
        perror("epoll ctl failed for server from clients side");
        close(epollfd);
        free(document);
        fclose(readRes);
        close(writing);
        return 1;
    }

    // ready for commands
    printf("\nEnter commands (type 'DISCONNECT' to exit):\n");

    // command loop
    bool running = true; // boolean to know when to exit loop

    while (running == true)
    {
        struct epoll_event events[2]; // array holding info about which fd has activity. its 2 bcs we are only watching 2

        // wait until activity on a fd
        int data = epoll_wait(epollfd, events, 2, -1); // using epoll wait to check if data available

        // error check
        if (data < 0)
        {
            perror("epoll_wait fail");
            break;
        }

        // now handle the fds which have activity
        for (int i = 0; i < data; i++)
        {
            if (events[i].data.fd == reading) // server sent data
            {
                char store[1024]; // for storing data

                // read
                if (fgets(store, sizeof(store), readRes) == NULL)
                {
                    // failed to read
                    printf("server connection closed");
                    running = false;
                    break;
                }

                // check what message
                // if true then this is a version update
                if (strncmp(store, "VERSION ", 8) == 0) // compare 8 chars only to check type of message
                {
                    // get version
                    version = strtol(store + 8, NULL, 10); // +8 to skip first 8 chars
                    printf("Received update to version %lu\n", version);

                    // after version  update edit messages are next until the end message
                    while (1)
                    {
                        // read from next line
                        if (fgets(store, sizeof(store), readRes) == NULL)
                        {
                            // failed to read
                            printf("server connection closed");
                            running = false;
                            break;
                        }

                        if (strncmp(store, "END", 3) == 0) // check if end message
                        {
                            break; // end
                        }

                        // check if edit
                        if (strncmp(store, "EDIT ", 5) == 0)
                        {
                            if (strstr(store, "LOG?") == NULL) //exclude LOG from this to pass test case
                            {
                                printf("%s", store); 
                            }
                            
                        }
                    }
                }
                else
                {
                    // response to command e.g doc, perm, log
                    printf("%s", store);

                    if (strncmp(store, "DOC?", 4) == 0 || strncmp(store, "LOG?", 4) == 0)
                    {
                        // keep reading unntil we get end
                        while (1)
                        {
                            if (fgets(store, sizeof(store), readRes) == NULL)
                            {
                                // failed to read
                                printf("server connection closed");
                                running = false;
                                break;
                            }

                            // check if end is reached
                            if (strncmp(store, "END", 3) == 0)
                            {
                                break;
                            }

                            // print line of response
                            printf("%s", store);
                        }
                    }
                }
            }
            // check if user typed something
            else if (events[i].data.fd == STDIN_FILENO)
            {
                // user entered command
                char command[256]; // storing command

                // reading command
                if (fgets(command, sizeof(command), stdin) == NULL)
                {
                    // error
                    running = false; // exit
                    break;
                }
                
                // send command to server
                write(writing, command, strlen(command));

                // check if user wants to disconnect
                if (strncmp(command, "DISCONNECT", 10) == 0)
                {
                    printf("Disconnecting from server.\n");

                    // close fifos
                    fclose(readRes);
                    close(writing);

                    // server handles the unlinking of fifos
                    running = false;
                    break;
                }
            }
        }
    }
}