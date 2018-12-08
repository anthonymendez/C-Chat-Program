/*
 * echoclient.c - An echo client
 */
/* $begin echoclientmain */
#include "csapp.h"

/* Type of Threads Client will run */
enum ThreadType {Sender, Receiver};

int clientfd;
char *host, *port, buf[MAXLINE];
rio_t rio;
int killProgram;

void *thread(void *vargp);
void senderRoutine();
void receiverRoutine();
void sendDataToServer();

int main(int argc, char **argv) {
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
    printf("SenderRoutineThread\n");
}

/* Routine to listen to message from the server */
void receiverRoutine() {
    printf("ReceivRoutineThread\n");
}

/* Function to quickly send data in buffer to server */
void sendDataToServer() {
    Rio_writen(clientfd, buf, strlen(buf));
}

/* Clear String Buffer */
void clearBuffer() {
    for(int i = 0; i < MAXLINE; i++) {
        buf[i] = 0;
    }
}
/* $end echoclientmain */
