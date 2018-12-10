/* 
 * echoservert.c - A concurrent echo server using threads
 */
/* $begin echoservertmain */
#include "csapp.h"

struct clientInfo {
    struct clientInfo* next;
    struct clientInfo* prev;
    char* username;
    int connfd;
};

char *userList();
void dropClient(struct clientInfo*);
void *thread(void*);

struct clientInfo* firstClient = NULL;

struct clientInfo* addClient(int connfd) {
    struct clientInfo* newClient = (struct clientInfo*) malloc(sizeof(struct clientInfo));
    newClient->connfd = connfd;
    
    if(firstClient == NULL) {
        //TODO: Mutex start?
        firstClient = newClient;
        firstClient->prev = NULL;
        firstClient->next = NULL;
        //TODO: Mutex end?
    } else {
        struct clientInfo* lastClient = firstClient;
        struct clientInfo* prevClient;
        while(lastClient->next != NULL) {
            prevClient = lastClient;
            lastClient = lastClient->next;
            lastClient->prev = prevClient;
        }
        //TODO: Mutex start?
        newClient->prev = lastClient;
        newClient->next = NULL;
        lastClient->next = newClient;
        //TODO: Mutex end?
    }
    
    return newClient;
}

void sendDirectMsg(const char* cmd) {
    // TODO: Write this function! Possible layout commented below:

    char* destUsr;
    char* msg;
    // Parse to destination username and msg (with error checking)
    
    // Check if destination user exists
    
    int dest_connfd;
    // Get socket(?) by username (iterate over linked list from firstClient?)
    // Is there more too it than the connfd to pass into Rio_writen?
    
    char* reply;
    // Formulate and send message
    //Rio_writen(dest_connfd, reply, strlen(reply));
}

// TODO: This is actually un-used for now... whoops. delete later if not needed
// Returns non-zero only if str begins with all of prefix
int hasPrefix(const char* str, const char* prefix) {
    size_t strLen = strlen(str);
    size_t preLen = strlen(prefix);
    
    // Compare strings, make sure their lengths are non-zero, and make sure prefix isn't longer than str
    return (strncmp(prefix, str, preLen) == 0 && strLen >= preLen && strLen > 0 && preLen > 0);
}

// Returns non-zero only if quit command was received
int handleMessage(const char* cmd, int len, int connfd) {
    // TODO: Test if statements' conditions
    if(len == 4 && strncmp("quit\0", cmd, 5) == 0) { // Quit
        printf("[debug] quit received\n");
        return 1;
    } else if(len == 10 && strncmp("list-users\0", cmd, 11) == 0) { // List users
        // TODO: Test to make sure client receives properly
        printf("[debug] list-users received\n");
        char* userListString = userList();
        Rio_writen(connfd, userListString, strlen(userListString));
        free(userListString);
    } else if(cmd[0] == '@') { // Direct msg
        printf("[debug] direct message received\n");
        sendDirectMsg(cmd);
    } else { // Invalid command (not quit, list-users, or @ (direct msg))
        printf("[debug] invalid command received\n");
        // TODO: Handle
    }
    return 0;
}

/* Clear a given buffer */
void clearBuffer(char buf[], int len) {
    for(int i = 0; i < len; i++) {
        buf[i] = 0;
    }
}

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
    printf("[debug] -username- received: -%s-\n", info->username);


    // TODO: any more initialization I forgot?
    // Main loop handling client
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) { // TODO: Should this break the loop?
        printf("[debug] len: %d, -msg-: -%s-\n", n, buf);
        buf[n - 1] = '\0'; // Clear trailing newline character
        if(handleMessage(buf, n - 1, connfd) != 0) // Quit command received
            break;
        clearBuffer(buf, MAXLINE);
    }

    printf("[debug] loop broken, client quit (or disconnected/closed?)\n");
    dropClient(info);
    return;

    /*
    //Example input receiving/send echo loop
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %d bytes\n", n);
        Rio_writen(connfd, buf, n);
    }
    */
}

/* Iterate through clients and concatenate clients usernames to string 
 * Returns string like so
 * "@Ant
 *  @Tony
 *  @Josh
 * "
 */
char* userList() {
    struct clientInfo *clientNode = firstClient;
    char *userListString = calloc(MAXLINE, sizeof(char));
    while (clientNode != NULL) {
        sprintf(userListString, "%s@%s\n", userListString, clientNode->username);
        clientNode = clientNode->next;
    }
    return userListString;
}

void dropClient(struct clientInfo* client) {
    /* Check for null pointer */
    if (client == NULL)
        return;

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
}

int main(int argc, char **argv) 
{
    printf("[debug] MAXLINE=%d\n", MAXLINE);
    
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
