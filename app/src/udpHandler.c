#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>

#include "udpHandler.h"

#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <string.h>

// Put it on port 12345
#define PORT 12345
#define MAX_LEN 8192

static struct sockaddr_in socket_in;
static struct sockaddr_in socket_remote;
static int socketDescriptor;

static bool is_initialized = false;

// stuff from the lecture slides, notes from cmpt 218, and youtube explainations
// had to review this pretty heavily

// fills in the address information
void udpHandler_init(){
    assert(!is_initialized);
    printf("Initializing UDP socket on port: %d...\n", PORT);

    //set the memory of it
    memset(&socket_in, 0, sizeof(socket_in));
    socket_in.sin_family = AF_INET;
    socket_in.sin_addr.s_addr = htonl(INADDR_ANY);
    socket_in.sin_port = htons(PORT);

    // then bind it to the socket
    socketDescriptor = socket(PF_INET, SOCK_DGRAM, 0);

    // check if making the descriptor was successful
    if(socketDescriptor < 0){
        perror("socket creation failed!");
        exit(EXIT_FAILURE);
    }

    // if it is, bind it
    int bindResult = bind(socketDescriptor, (struct sockaddr*) &socket_in, sizeof(socket_in));

    // check if the bind succeeded
    if(bindResult < 0){
        perror("socket bind failed");
        exit(EXIT_FAILURE);
    }
}


void udpHandler_recieve(char* msgRetrieved){
    // listen in and see if anything came in
    unsigned int remoteLen = sizeof(socket_remote);

    // retrieve the message
    //printf("Waiting to receive data...\n");
    int bytesRetreived = recvfrom(socketDescriptor, msgRetrieved, MAX_LEN - 1, 0, (struct sockaddr*) &socket_remote, &remoteLen);
    //printf("received data...\n");

    // if failed to recieve anything
    if (bytesRetreived < 0) {
        perror("recvfrom failed");
    }

    // check to make sure it's less than the max length
    // if it isn't just cut it off
    if(bytesRetreived >= MAX_LEN){
        bytesRetreived = MAX_LEN - 1;
    }

    // add in the null terminator at the end
    msgRetrieved[bytesRetreived] = 0;

    // TODO: process the message
    //printf("UDP msg: %s, len: %d", msgRetrieved, bytesRetreived);
}


void udpHandler_reply(char* msg){
    unsigned int remoteLen = sizeof(socket_remote);

    // send that message back to the remote
    sendto(socketDescriptor, msg, strlen(msg), 0, (struct sockaddr*) &socket_remote, remoteLen);
}



void udpHandler_close(){
    // just close our socket
    printf("Closing UDP socket on port: %d...\n", PORT);
    close(socketDescriptor);
}