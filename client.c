/*
 * echoclient.c - An echo client
 */
/* $begin echoclientmain */
#include "csapp.h"

#define QUIT "quit"
#define LIST_USERS "list-users"

/* Type of Threads Client will run */
enum ThreadType {Sender, Receiver};

int clientfd;
char *host, *port;
rio_t rio;
int killProgram;

void *thread(void *vargp);
void senderRoutine();
void receiverRoutine();
void sendDataToServer(char[]);
void clearBuffer(char[], int);

int main(int argc, char **argv) {
    char buf[MAXLINE];
    pthread_t tid;

    if (argc != 4) {
        fprintf(stderr, "usage: %s <host> <port> <username>\n", argv[0]);
	    exit(0);
    }
    host = argv[1];
    port = argv[2];

    killProgram = 0;
    enum ThreadType S = Sender,
                    R = Receiver;

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);

    strcat(buf, argv[3]);
    strcat(buf, "\n");
    Rio_writen(clientfd, buf, strlen(buf)); // Send username immediately

    printf("[debug] -username- sent (before newline): -%s-\n", argv[3]);

    Pthread_create(&tid, NULL, thread, &S);
    Pthread_create(&tid, NULL, thread, &R);

    while(!killProgram); //TODO: Remove/replace with code
    
    //TODO: make another thread, have one do sending, one do receiving, or whatever the website said
    
    /*
    //Example input sending/receiving echo loop
    while (Fgets(buf, MAXLINE, stdin) != NULL) {
	Rio_writen(clientfd, buf, strlen(buf));
	Rio_readlineb(&rio, buf, MAXLINE);
	Fputs(buf, stdout);
    }
    */
    
    Close(clientfd); //line:netp:echoclient:close
    exit(0);
}

/* Thread routine */
void *thread(void *vargp) {
    enum ThreadType threadType = *((enum ThreadType*) vargp);
    Pthread_detach(pthread_self()); //line:conc:echoservert:detach

    // Launch Appropriate ThreadType
    if (threadType == Sender) {
        senderRoutine();
    } else if (threadType == Receiver) {
        receiverRoutine();
    } else {
        // Shouldn't happen, exit program
        fprintf(stderr, "Tried to launch invalid thread type\n");
        exit(0);
    }
    return NULL;
}

/* Routine to reciever input and send messages to the server */
void senderRoutine() {
    char buf[MAXLINE];

    while (!killProgram && Fgets(buf, MAXLINE, stdin) != NULL) {
        if (strcmp(buf, QUIT) == 0) {
            killProgram = 1;
        }
        sendDataToServer(buf);
        clearBuffer(buf, MAXLINE);
    }
}

/* Routine to listen to message from the server */
void receiverRoutine() {
    char buf[MAXLINE];

    while (!killProgram) {
        Rio_readlineb(&rio, buf, MAXLINE);
        fprintf(stdout, "%s\n", buf);
        clearBuffer(buf, MAXLINE);
    }
}

/* Function to quickly send data in buffer to server */
void sendDataToServer(char buf[]) {
    Rio_writen(clientfd, buf, strlen(buf));
}

/* Clear a given buffer */
void clearBuffer(char buf[], int len) {
    for(int i = 0; i < len; i++) {
        buf[i] = 0;
    }
}
/* $end echoclientmain */
