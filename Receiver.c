#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <stdlib.h>

//constants
#define SERVER_PORT 9999
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 100000
#define arrLen 1000

//methods signatures
void receiveFilesFlow(double *arr, int i, int serverSocket, int clientSocket);

int receive(int clientSocket);

int sendAuth(int serverSocket, int clientSocket);

void calculateAverage(double *arr);

int serverSocketSetup();

void changeCCcubic(int clientSocket);

void changeCCreno(int clientSocket);

int main() {
    //init the arr that holds the elapsed time
    double arr[1000];
    int i = 0;
    int serverSocket = serverSocketSetup();
    printf("Waiting for incoming TCP-connections...\n");
    struct sockaddr_in clientAddress;  //
    socklen_t clientAddressLen = sizeof(clientAddress);
    while (1) {
        memset(&clientAddress, 0, sizeof(clientAddress));
        clientAddressLen = sizeof(clientAddress);
        //Accepting a new client connection
        int clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddress, &clientAddressLen);
        if (clientSocket == -1) {
            printf("listen failed with error code : %d\n", errno);
            close(serverSocket);
            return -1;
        }
        printf("A new client connection accepted\n");
        receiveFilesFlow(arr, i, serverSocket, clientSocket);
        close(clientSocket);
    }
    close(serverSocket);
    return 0;
}

int serverSocketSetup() {
    int serverSocket;
    //Opening a new TCP socket
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Failed to open a TCP connection : %d", errno);
        return -1;
    }
    //Enabling reuse of the port
    int enableReuse = 1;
    int ret = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int));
    if (ret < 0) {
        printf("setSockopt() failed with error code : %d", errno);
        return -1;
    }
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddress.sin_port = htons(SERVER_PORT);
    //bind() associates the socket with its local address 127.0.0.1
    int bindResult = bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    if (bindResult == -1) {
        printf("Bind failed with error code : %d\n", errno);
        close(serverSocket);
        return -1;
    }
    //Preparing to accept new in coming requests
    int listenResult = listen(serverSocket, 10);
    if (listenResult == -1) {
        printf("Bind failed with error code : %d\n", errno);
        return -1;
    }
    return serverSocket;
}

//A method that does all the files flow, sending the authentication to the sender and checking for exit messages
void receiveFilesFlow(double *arr, int i, int serverSocket, int clientSocket) {
    int j = 0;
    char exitMessage[2] = "n";
    char resendMessage[2] = "y";
    while (strcmp(exitMessage, "n") == 0) {
        while (strcmp(resendMessage, "y") == 0) {
            //Setting the CC algorithm to cubic
            changeCCcubic(clientSocket);
            //Receiving the first part of the file and checking the start and end time
            printf("Receiving first part... \n");
            struct timeval start, end;
            double elapsed;
            gettimeofday(&start, NULL);
            if (receive(clientSocket) == -1) {
                exit(1);
            }
            gettimeofday(&end, NULL);
            elapsed = (double) (end.tv_sec - start.tv_sec) * 1e6;
            elapsed = (elapsed + (double) (end.tv_usec - start.tv_usec)) * 1e-6;
            printf("time elapsed %f in seconds - cubic cc algorithm \n", elapsed);
            arr[i] = elapsed;
            i++;
            printf("First part Received \n");
            //Sending the Authentication message to the sender
            if (sendAuth(serverSocket, clientSocket) == -1) {
                printf("Error sending the authentication");
            }
            //Setting the CC algorithm to reno
            changeCCreno(clientSocket);
            //Receiving the first part of the file and checking the start and end time
            printf("Receiving second part...\n");
            gettimeofday(&start, NULL);
            if (receive(clientSocket) == -1) {
                exit(1);
            }
            gettimeofday(&end, NULL);
            elapsed = (double) (end.tv_sec - start.tv_sec) * 1e6;
            elapsed = (elapsed + (double) (end.tv_usec - start.tv_usec)) * 1e-6;
            printf("time elapsed %f in seconds - reno cc algorithm  \n", elapsed);
            arr[i] = elapsed;
            i++;
            printf("Second part Received \n");
            j++;
            int sendAgain = recv(clientSocket, resendMessage, 2, 0);
            if (sendAgain == -1) {
                printf("receiving resendMessage error\n");
            }
        }
        int clientChoice = recv(clientSocket, exitMessage, 2, 0);
        if (clientChoice == -1) {
            printf("Error in receiving exit message");
        }
        if (strcmp(exitMessage, "y") == 0) {
            calculateAverage(arr);
        }
    }
    printf("received %d files\n", j);
}


void changeCCreno(int clientSocket) {
    if (setsockopt(clientSocket, IPPROTO_TCP, TCP_CONGESTION, "reno", 4) < 0) {
        printf("set socket error from client\n");
    }
}

void changeCCcubic(int clientSocket) {
    if (setsockopt(clientSocket, IPPROTO_TCP, TCP_CONGESTION, "cubic", 5) < 0) {
        printf("set socket error from client\n");
    }
}

//this method handles receiving the files from the sender
int receive(int clientSocket) {
    char bufferReply[BUFFER_SIZE];
    bzero(bufferReply, sizeof(bufferReply));
    size_t total = 0;
    while (1) {
        int bytes = recv(clientSocket, bufferReply, 100000, 0);
        if (bytes == -1) {
            printf("receive failed\n");
            return -1;
        } else total += bytes;
        if (bytes == 0 || total == 528385 || total == 528387) {
            break;
        }
    }
    return 1;
}

int sendAuth(int serverSocket, int clientSocket) {
    //Sending the Authentication message to the sender
    char idXor[] = "1110110010010";
    size_t len = sizeof(idXor);
    int bytesSent = send(clientSocket, idXor, len, 0);
    if (bytesSent == -1) {
        printf("send() failed with error code : %d \n", errno);
        close(serverSocket);
        close(clientSocket);
        return -1;
    }
    printf("Authentication sent\n");
    return 1;
}

//this method calculate the average time that took the files sent from the sender to the receiver
void calculateAverage(double *arr) {
    double sumFirstPart = 0;
    double sumSecondPart = 0;
    int i;
    for (i = 0; i < arrLen; i++) {
        if (arr[i] == 0) break;
        printf("Time [%f]\n", arr[i]);
        if (i % 2 == 0)sumFirstPart += arr[i];
        if (i % 2 == 1)sumSecondPart += arr[i];
    }
    printf("the average time for the first parts is %f with cc=cubic \n", sumFirstPart / ((double) i / 2));
    printf("the average time for the second parts is %f with cc=reno \n", sumSecondPart / ((double) i / 2));
}

