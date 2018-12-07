/*
 * echoclient.c - An echo client
 */
/* $begin echoclientmain */
#include "csapp.h"

int main(int argc, char **argv) 
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;
    if (argc != 4) {
	fprintf(stderr, "usage: %s <host> <port> <username>\n", argv[0]);
	exit(0);
    }
    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);
    strcat(buf, argv[3]);
    strcat(buf, "\n");
    Rio_writen(clientfd, buf, strlen(buf)); // Send username immediately
    printf("[debug] -username- sent (before newline): -%s-\n", argv[3]);
    while(1); //TODO: Remove/replace with code
    
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
/* $end echoclientmain */
