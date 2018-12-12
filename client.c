/*
 * echoclient.c - An echo client
 */
/* $begin echoclientmain */
#include "csapp.h"

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
    /* Check if we have the appropriate arguments */
    if (argc != 4) {
        fprintf(stderr, "usage: %s <host> <port> <username>\n", argv[0]);
	    exit(0);
    }

    char buf[MAXLINE];
    pthread_t tid;

    /* Connect to the server */
    host = argv[1];
    port = argv[2];
    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);

    killProgram = 0;
    /* Start receiverRoutine() in a new detached thread */
    Pthread_create(&tid, NULL, thread, NULL);

    /* Check if username provided is too long to be sent
     * Subtracting 2 to account for \0 charactere
     */
    if (strlen(argv[3]) > MAXLINE - 2) {
        fprintf(stderr, "Username too long!\n");
	    exit(0);
    }

    /* Send username immediately */
    strcat(buf, argv[3]);
    strcat(buf, "\n");
    Rio_writen(clientfd, buf, strlen(buf));

    /* This thread becomes dedicated for senderRoutine until we quit */
    senderRoutine();

    /* Close connection and exit program */
    Close(clientfd); //line:netp:echoclient:close
    exit(0);
}

/* Thread routine */
void *thread(void *vargp) {
    Pthread_detach(pthread_self()); //line:conc:echoservert:detach
    receiverRoutine();
    return NULL;
}

/* Routine to reciever input and send messages to the server */
void senderRoutine() {
    char buf[MAXLINE];

    printf("> ");
    fflush(stdout);
    // Note: Fgets puts a trailing \n\0 into buf (after the text entered by user)
    while (!killProgram && Fgets(buf, MAXLINE, stdin) != NULL) {
        sendDataToServer(buf);

        // If buf begins with "quit" followed by \n\0, then quit
        if (strncmp("quit\n\0", buf, 6) == 0) {
            killProgram = 1;
            printf("GoodBye!\n");
            break;
        }

        clearBuffer(buf, MAXLINE);
        printf("> "); // Fgets blocks until newline, so it won't be needed here
        fflush(stdout);
    }
}

/* Routine to listen to message from the server */
void receiverRoutine() {
    char buf[MAXLINE];

    while (!killProgram) {
        Rio_readlineb(&rio, buf, MAXLINE);

        // Maybe don't handle that case? See https://piazza.com/class/jlqsbkrvhww31w?cid=169
        printf("\b\b%s> ", buf); // Backspace the "> ", print buf, and add "> "
        fflush(stdout);
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
