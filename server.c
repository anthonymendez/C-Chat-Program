/* 
 * echoservert.c - A concurrent echo server using threads
 */
/* $begin echoservertmain */
#include "csapp.h"

#define INVALID_CMD "Invalid Command Received\n"
#define INVALID_USERNAME "Username missing @\n"
#define USERNAME_NOT_FOUND "Username not found!\n"

#define INVALID_CMD_LEN strlen(INVALID_CMD)
#define INVALID_USERNAME_LEN strlen(INVALID_USERNAME)
#define USERNAME_NOT_FOUND_LEN strlen(USERNAME_NOT_FOUND)

/* Doubly Linked List to store Client Info */
struct clientInfo {
    struct clientInfo* next;
    struct clientInfo* prev;
    char* username;
    int connfd;
};

/* Function Prototypes */
void lockMutex();
void unlockMutex();
struct clientInfo* addClient(int connfd);
void sendDirectMsg(const char* cmd, char* senderUsername, int connfd);
int handleMessage(const char* cmd, int len, int connfd, char* fromUsername);
void clearBuffer(char buf[], int len);
void clientHandlerStart(struct clientInfo* info);
char *userList();
void dropClient(struct clientInfo*);
void *thread(void*);

/* Mutex to make sure threads don't access client info list
 * at the same time.
 */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Head of our client info doubly linked list */
struct clientInfo* firstClient = NULL;

/* Locks the mutex or waits until it can lock the mutex */
void lockMutex() {
    pthread_mutex_lock(&mutex);
}

/* Unlocks the mutex */
void unlockMutex() {
    pthread_mutex_unlock(&mutex);
}

/* Adds a client to our doubly linked client info list */
struct clientInfo* addClient(int connfd) {
    struct clientInfo* newClient = (struct clientInfo*) malloc(sizeof(struct clientInfo));
    newClient->connfd = connfd;

    lockMutex();
    if (firstClient == NULL) {
        firstClient = newClient;
        firstClient->prev = NULL;
        firstClient->next = NULL;
    } else {
        struct clientInfo* lastClient = firstClient;
        struct clientInfo* prevClient;
        while(lastClient->next != NULL) {
            prevClient = lastClient;
            lastClient = lastClient->next;
            lastClient->prev = prevClient;
        }
        newClient->prev = lastClient;
        newClient->next = NULL;
        lastClient->next = newClient;
    }
    unlockMutex();

    return newClient;
}

/* Sends a message directly to the client specified in CMD */
void sendDirectMsg(const char* cmd, char* senderUsername, int connfd) {
    char* destUsr;
    char* msg = calloc(MAXLINE, sizeof(char));
    strcpy(msg, cmd);

    // Parse to destination username and msg (with error checking)
    char* token = strtok(msg, " ");
    if (token[0] != '@') {
        Rio_writen(connfd, INVALID_USERNAME, INVALID_USERNAME_LEN);
        return;
    }
    destUsr = token + 1;

    // Check if destination user exists
    lockMutex();
    struct clientInfo* search = firstClient;
    while(search != NULL  && strcmp(search->username, destUsr) != 0) {
        search = search->next;
    }

    // Check if we found the username
    if(search == NULL) {
        Rio_writen(connfd, USERNAME_NOT_FOUND, USERNAME_NOT_FOUND_LEN);
        unlockMutex();
        return;
    }

    // Get socket from clientInfo  
    int dest_connfd = search->connfd;

    unlockMutex();

    // Get message from cmd
    char* transmit = calloc(MAXLINE, sizeof(char));
    sprintf(transmit, "@%s %s\n", senderUsername, cmd+strlen(token)+1);

    // Send message
    Rio_writen(dest_connfd, transmit, strlen(transmit));
    free(transmit);
    free(msg);
}

// Returns non-zero only if quit command was received
int handleMessage(const char* cmd, int len, int connfd, char* fromUsername) {
    /* User wants to disconnect from server */
    if(len == 4 && strncmp("quit\0", cmd, 5) == 0) {
        return 1;
    }
    /* User wants to get a list of users connected to server */
    else if(len == 10 && strncmp("list-users\0", cmd, 11) == 0) {
        char* userListString = userList();
        Rio_writen(connfd, userListString, strlen(userListString));
        free(userListString);
    }
    /* User wants to send a message to a user connected to the server */
    else if(cmd[0] == '@') {
        sendDirectMsg(cmd, fromUsername, connfd);
    }
    /* User sent an invalid command */
    else {
        Rio_writen(connfd, INVALID_CMD, INVALID_CMD_LEN);
    }
    return 0;
}

/* Clear a given buffer */
void clearBuffer(char buf[], int len) {
    for(int i = 0; i < len; i++) {
        buf[i] = 0;
    }
}

/* Handles starting reading and writing loop to client
 */
void clientHandlerStart(struct clientInfo* info)
{
    int connfd = info->connfd;
    int n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    n = Rio_readlineb(&rio, buf, MAXLINE); // Capture username that client immediately sends
    
    // Copy username from buffer, except for the trailing newline
    char* usrSlot = malloc(sizeof(char) * n); // Only n (not n+1) because stripping newline
    for(int i=0; i<n-1 && i<MAXLINE; i++)
        usrSlot[i] = buf[i];
    usrSlot[n-1] = '\0'; // Add string terminator
    info->username = usrSlot;
    clearBuffer(buf, MAXLINE);
    printf("@%s joined\n", info->username);


    // Main loop handling client
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        buf[n - 1] = '\0'; // Clear trailing newline character
        if(handleMessage(buf, n - 1, connfd, info->username) != 0) // Quit command received
            break;
        clearBuffer(buf, MAXLINE);
    }

    printf("@%s left\n", info->username);
    dropClient(info);
}

/* Iterate through clients and concatenate clients usernames to string 
 * Returns string like so
 * "@Ant
 *  @Tony
 *  @Josh
 * "
 */
char* userList() {
    lockMutex();
    struct clientInfo *clientNode = firstClient;
    char *userListString = calloc(MAXLINE, sizeof(char));
    while (clientNode != NULL) {
        sprintf(userListString, "%s@%s\n", userListString, clientNode->username);
        clientNode = clientNode->next;
    }
    unlockMutex();
    return userListString;
}

/* Drops specified client from list */
void dropClient(struct clientInfo* client) {
    lockMutex();

    /* Check for null pointer */
    if (client == NULL) {
        unlockMutex();
        return;
    }

    /* Check if client is head node */
    if (client == firstClient) {
        firstClient = client->next;
    }

    /* Set next's prev node equal to prev node */
    if (client->next != NULL) {
        client->next->prev = client->prev;
    }

    /* Set prev's next node equal to next node */
    if (client->prev != NULL) {
        client->prev->next = client->next;
    }

    unlockMutex();
}

int main(int argc, char **argv) {
    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid; 

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
	    exit(0);
    }
    listenfd = Open_listenfd(argv[1]);

    while (1) {
        clientlen=sizeof(struct sockaddr_storage);
        connfdp = Malloc(sizeof(int)); //line:conc:echoservert:beginmalloc
        *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen); //line:conc:echoservert:endmalloc
        struct clientInfo* newClient = addClient(*connfdp);
        Free(connfdp);                    //line:conc:echoservert:free
        Pthread_create(&tid, NULL, thread, &newClient);
    }
}

/* Thread routine */
void *thread(void *vargp) 
{
    struct clientInfo* newClient = (struct clientInfo*) *((struct clientInfo**) vargp);
    Pthread_detach(pthread_self()); //line:conc:echoservert:detach
    clientHandlerStart(newClient);
    Close(newClient->connfd);
    free(newClient);
    return NULL;
}
/* $end echoservertmain */
