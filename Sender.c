#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <stdlib.h>

//constants
#define SERVER_PORT 9999
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 100000

//methods signatures
void sendFile(int sock);

int clientSocketSetup();

void changeToCubic(int sock);

void changeToReno(int sock);

int main() {
    //setting up the sender socket
    int senderSocket = clientSocketSetup();
    if (senderSocket == -1) {
        exit(-1);
    }
    printf("connected to server\n");
    //Waiting on user input - 1 to resend the files to the receiver and 0 to close the socket
    sendFile(senderSocket);
    return 0;
}

int clientSocketSetup() {
    // Opening a new socket connection
    int senderSocket = 0;
    if ((senderSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Failed to open a TCP connection : %d", errno);
        return -1;
    }
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    int binaryAddress = inet_pton(AF_INET, (const char *) SERVER_IP, &serverAddress.sin_addr);
    if (binaryAddress <= 0) {
        printf("Failed to convert from text to binary : %d", errno);
    }
    // Connecting to the receiver's socket
    int connection = connect(senderSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    if (connection == -1) {
        printf("Connection error : %d\n", errno);
        close(senderSocket);
        exit(1);
    }
    return senderSocket;
}

//this method handles sending the file to the receiver
void sendFile(int sock) {
    //Opening the file
    FILE *textFile;
    long counter = 0;
    if ((textFile = fopen("textfile.txt", "rb")) == NULL) {
        printf("opening file failed \n");
        fclose(textFile);
        exit(-1);
    }
    // counting how many characters there are in the file
    char c;
    while ((c = getc(textFile)) != EOF) {
        counter++;
    }
    rewind(textFile);
    // Splitting the file into 2 parts
    int half = (int) (counter / 2) + 2;
    char firstPart[half];
    char secondPart[half];
    fgets(firstPart, half, textFile);
    fgets(secondPart, half, textFile);
    fclose(textFile);
    // asking the user how many times he wants to send the file
    // 2 parts each time + Authentication check
    char userInput[2]="y";
    char exitMessage[2] = "n";
    while (strcmp(exitMessage, "n") == 0) {
        while (strcmp(userInput, "y") == 0) {
            //Setting the CC algorithm to cubic
            changeToCubic(sock);
            //Sending the first part of the file to the receiver
            size_t messageLen1 = sizeof(firstPart);
            ssize_t bytesSent = send(sock, firstPart, messageLen1, 0);
            if (bytesSent == -1) {
                printf("send() failed with error code : %d", errno);
            } else if (bytesSent == 0) {
                printf("peer has closed the TCP connection prior to send() :S.\n");
            } else if (bytesSent < counter / 2) {
                printf("sent only %d bytes from the required %zd.\n", half, bytesSent);
            } else {
                printf("First part was successfully sent %zd.\n", bytesSent);
            }
            //receiving the Authentication message from the receiver
            char bufferReply[14];
            int bytesReceived = recv(sock, bufferReply, sizeof(bufferReply), 0);
            if (bytesReceived == -1) {
                printf("recv() failed with error code : %d\n", errno);
            }
            printf("received %d bytes from server: %s\n", bytesReceived, bufferReply);
            // checking if the Authentication was successful
            char id[] = "1110110010010";
            int ans = strcmp(bufferReply, id);
            if (ans == 0) {
                printf("Authenticated successfully\n");
            }
            //Setting the CC algorithm to reno
            changeToReno(sock);
            //Sending the second part of the file to the receiver
            size_t messageLen2 = sizeof(secondPart);
            printf("this is second part %zu\n", messageLen2);
            int bytesSent2 = send(sock, secondPart, messageLen2, 0);
            if (bytesSent2 == -1) printf("send failed");
            if (bytesSent2 == -1) {
                printf("send() failed with error code : %d", errno);
            } else if (bytesSent2 == 0) {
                printf("peer has closed the TCP connection prior to send().\n");
            } else if (bytesSent2 < counter / 2) {
                printf("sent only %d bytes from the required %zd.\n", half, bytesSent);
            } else {
                printf("Second part was successfully sent %zd.\n", bytesSent);
            }
            printf("press y to resend the file or n to exit:");
            scanf(" %s", userInput);
            int messageLenAgain = sizeof(userInput);
            int bytesResend = send(sock, userInput, messageLenAgain, 0);
            if (bytesResend == -1) {
                printf("Error sending resend Message");
            }
        }
        printf("exit? press y to exit or n to continue:");
        scanf("%s", exitMessage);
        int messageLenExit = sizeof(exitMessage);
        int bytesSentExit = send(sock, exitMessage, messageLenExit, 0);
        if (bytesSentExit == -1) {
            printf("Error sending exit message");
        }
    }
    printf("Closing socket...");
    close(sock);
}


void changeToReno(int sock) {
    if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, "reno", 4) < 0) {
        printf("set socket error from client\n");
    }
}

void changeToCubic(int sock) {
    if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, "cubic", 5) < 0) {
        printf("set socket error from client\n");
    }
}
