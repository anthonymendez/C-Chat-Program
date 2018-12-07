/* 
 * echoservert.c - A concurrent echo server using threads
 */
/* $begin echoservertmain */
#include "csapp.h"

void *thread(void *vargp);

struct clientInfo {
    struct clientInfo* next;
    char* username;
    int connfd;
};

struct clientInfo* firstClient = NULL;

struct clientInfo* addClient(int connfd) {
    struct clientInfo* newClient = (struct clientInfo*) malloc(sizeof(struct clientInfo));
    newClient->connfd = connfd;
    
    if(firstClient == NULL) {
        //TODO: mutex start?
        firstClient = newClient;
        //TODO: mutex end?
    } else {
        struct clientInfo* lastClient = firstClient;
        while(lastClient->next != NULL) {
            lastClient = lastClient->next;
        }
        //TODO: mutex start?
        lastClient->next = newClient;
        //TODO: mutex end?
    }
    
    return newClient;
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
    
    printf("[debug] -username- received: -%s-\n", info->username);
    while(1); //TODO: Remove/replace with code
    
    //TODO: make loop to handle client (and any more initialization, if needed)
    
    
    /*
    //Example input receiving/send echo loop
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %d bytes\n", n);
        Rio_writen(connfd, buf, n);
    }
    */
}

int main(int argc, char **argv) 
{
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
    return NULL;
}
/* $end echoservertmain */
