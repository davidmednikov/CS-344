/*
// #####################################################
// Program 4 - OTP Encoder Daemon
// David Mednikov
// CS344 Spring 2019
//
// otp_enc_d
//
// This program runs in the background as a daemon. The
// program will listen on a particular port/socket to be
// determined by the user as a runtime argument.
// The program will receive a plaintext string and key
// which will be used to encode the plaintext into
// encrypted text. The encrypted string will be returned
// to the client.
// #####################################################
*/

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// define bools
typedef enum { false, true } bool;

/*
** encrypt Function
** ----------------------------------------------------
** function to encrypt text using a key
** params: string text, string key
** returns: string encryptedText
** ----------------------------------------------------
*/
char* encrypt(char* text, char* key) {
    // allocate int array of same length as text to be encoded
    int* textNums = (int*) calloc(strlen(text), sizeof(int));

    // iterator variable
    int i;

    // loop through text string, copying ascii code to textNums array
    for (i = 0; i < strlen(text); i++) {
        // if space, add 26 to text array
        if (text[i] == 32) {
            textNums[i] = 26;
        } else {
            // not space, subtract 65 from ascii code so that 0-25 are letter codes
            textNums[i] = text[i] - 65;
        }
    }

    // allocate int array of same length as key
    int* keyNums = (int*) calloc(strlen(key), sizeof(int));

    // loop through key string, copying ascii code to keyNums array
    for (i = 0; i < strlen(key); i++) {
        // if space, add 26 to key array
        if (key[i] == 32) {
            keyNums[i] = 26;
        } else {
            // not space, subtract 65 from ascii code so that 0-25 are letter codes
            keyNums[i] = key[i] - 65;
        }
    }

    // allocate int array of same length as text to be encoded
    int* modulo = (int*) calloc(strlen(text), sizeof(int));

    // loop through array, add char values from two arrays then modulo 27
    for (i = 0; i < strlen(text); i++) {
        modulo[i] = (textNums[i] + keyNums[i]) % 27;
    }

    // allocate memory for encrypted string
    char* encrypted = (char*) calloc(strlen(text), sizeof(char));

    // loop through length of text and add encrypted char to encrypted string
    for (i = 0; i < strlen(text); i++) {
        // get code of current char from modulo array
        int code = modulo[i];

        // current char code represents a space, set to 32 (ascii code for space)
        if (code == 26) {
            encrypted[i] = 32;
        } else {
            // not a space, add 65 to number to get ascii value
            encrypted[i] = modulo[i] + 65;
        }
    }

    // return encrypted string to calling function
    return encrypted;
}

/*
** error Function
** ----------------------------------------------------
** function to print a string to stderr
** params: string
** returns: none
** ----------------------------------------------------
*/
void error(const char* error) {
    fprintf(stderr, error);
}

/*
** main
** ----------------------------------------------------
** main function that accepts connections from clients
** and encodes the plaintext string using the provided key
** ----------------------------------------------------
*/
int main(int argc, char* argv[]) {
    // if incorrect number of arguments, display correct usage to user and quit
    if (argc != 2) {
        fprintf(stderr, "USAGE: otp_enc_d <listening_port>\n");
        return 1;
    }

    // sockets and port number
    int listenSocketFD;
    int establishedConnectionFD;
    int portNumber;

    // chars read, chars written, and child pid
    int charsRead;
    int charsWritten;
    int childPid;

    // socket conection variables and buffer
    socklen_t sizeOfClientInfo;
    char buffer[131072];
    struct sockaddr_in serverAddress;
    struct sockaddr_in clientAddress;

    // set up the server address struct
    memset((char *) &serverAddress, '\0', sizeof(serverAddress)); // clear out server address struct
    portNumber = atoi(argv[1]); // get port number
    serverAddress.sin_family = AF_INET; // create network-capable socket
    serverAddress.sin_port = htons(portNumber); // store port number
    serverAddress.sin_addr.s_addr = INADDR_ANY; // accept connections from any address

    // set up socket
    listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocketFD < 0) {
        error("SERVER: ERROR opening socket\n");
        exit(1);
    }

    // bind the socket so that we can start listening
    if (bind(listenSocketFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        error("SERVER: ERROR binding socket\n");
        exit(1);
    }

    // listen on the socket and allow up to 5 connections
    listen(listenSocketFD, 5);

    // keep looping
    while (true) {
        // accept a connection, blocking if one is not available until one connects
        sizeOfClientInfo = sizeof(clientAddress); // get size of address where client will connect
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *) &clientAddress, &sizeOfClientInfo);

        // error when accepting conection
        if (establishedConnectionFD < 0) {
            error("SERVER: ERROR accepting connection from socket\n");
            exit(1);
        }

        // connection successful, create new thread
        childPid = fork();

        // child process, handle rest of transaction here
        if (childPid == 0) {
            char* plaintext;
            char* key;

            // clear out buffer
            memset(buffer, '\0', 131072);

            // receive data from socket
            charsRead = recv(establishedConnectionFD, buffer, 131071, 0);
            if (charsRead < 0) {
                // error receiving data from socket
                error("SERVER: ERROR receiving data from socket\n");
            }

            // no error, make sure connection originates from otp_enc
            if (strcmp(buffer, "enc") == 0) {
                // connection is from otp_enc, clear out buffer and set to "enc_d" to send back to client
                memset(buffer, '\0', 131072);
                strcpy(buffer, "enc_d");

                // send buffer to client
                charsWritten = send(establishedConnectionFD, buffer, strlen(buffer), 0);
                if (charsWritten < 0) {
                    // error sending data to socket
                    error("SERVER: ERROR sending data to socket\n");
                }

                // no error, get length of plaintext from client

                // clear buffer and receive plaintext length from socket
                memset(buffer, '\0', 131072);
                charsRead = recv(establishedConnectionFD, buffer, 131071, 0);

                if (charsRead < 0) {
                    // error receiving data from socket
                    error("SERVER: ERROR receiving data from socket\n");
                }

                // no error, get length of plaintext and return to client for verification
                int plaintextLength = atoi(buffer);
                memset(buffer, '\0', 131072);
                sprintf(buffer, "%d", plaintextLength);

                // send length back to client
                charsWritten = send(establishedConnectionFD, buffer, strlen(buffer), 0);
                if (charsWritten < 0) {
                    // error sending data to socket
                    error("SERVER: ERROR sending data to socket\n");
                }

                // variables to store plaintext and keep track of received characters
                plaintext = calloc(plaintextLength + 1, sizeof(char));
                int charsReceived = 0;

                // while still receiving characters
                while (charsReceived < plaintextLength) {
                    // clear out buffer and receive data from socket
                    memset(buffer, '\0', 131072);
                    charsRead = recv(establishedConnectionFD, buffer, 131071, 0);
                    if (charsRead < 0) {
                        // error receiving data from socket
                        error("SERVER: ERROR receiving data from socket\n");
                    }

                    // no error, cat buffer to plaintext and increment chars received
                    strcat(plaintext, buffer);
                    charsReceived += strlen(buffer);
                }

                // clear buffer and set to number of received characters
                memset(buffer, '\0', 131072);
                sprintf(buffer, "%d", charsReceived);

                // send length back to client
                charsWritten = send(establishedConnectionFD, buffer, strlen(buffer), 0);
                if (charsWritten < 0) {
                    // error sending data to socket
                    error("SERVER: ERROR sending data to socket\n");
                }

                // clear buffer and receive key length from socket
                memset(buffer, '\0', 131072);
                charsRead = recv(establishedConnectionFD, buffer, 131071, 0);
                if (charsRead < 0) {
                    // error receiving data from socket
                    error("SERVER: ERROR receiving data from socket\n");
                }

                // no error, get length of key and return to client for verification
                int keyLength = atoi(buffer);
                memset(buffer, '\0', 131072);
                sprintf(buffer, "%d", keyLength);

                // send length back to client
                charsWritten = send(establishedConnectionFD, buffer, strlen(buffer), 0);
                if (charsWritten < 0) {
                    // error sending data to socket
                    error("SERVER: ERROR sending data to socket\n");
                }

                // create variables to store key and keep track of received characters
                key = calloc(keyLength + 1, sizeof(char));
                charsReceived = 0;

                // while still receiving characters
                while (charsReceived < keyLength) {
                    // clear out buffer and receive data from socket
                    memset(buffer, '\0', 131072);
                    charsRead = recv(establishedConnectionFD, buffer, 131071, 0);
                    if (charsRead < 0) {
                        // error receiving data from socket
                        error("SERVER: ERROR receiving data from socket\n");
                    }

                    // no error, cat buffer to key and increment chars received
                    strcat(key, buffer);
                    charsReceived += strlen(buffer);
                }

                // clear buffer and set to number of received characters
                memset(buffer, '\0', 131072);
                sprintf(buffer, "%d", charsReceived);

                // send length back to client
                charsWritten = send(establishedConnectionFD, buffer, strlen(buffer), 0);
                if (charsWritten < 0) {
                    // error sending data to socket
                    error("SERVER: ERROR sending data to socket\n");
                }

                // encrypt plaintext and copy to buffer to send back to client
                memset(buffer, '\0', 131072);
                strcpy(buffer, encrypt(plaintext, key));

                // send encrypted string back to client
                charsWritten = send(establishedConnectionFD, buffer, strlen(buffer), 0);
                if (charsWritten < 0) {
                    // error sending data to socket
                    error("SERVER: ERROR sending data to socket\n");
                }
            } else {
                // connection did not originate from otp_enc, ignore
                error("This connection did not originate from otp_enc");
            }

            // close communication socket
            close(establishedConnectionFD);

            // exit child process with status 0
            exit(0);
        }
    }

    close(listenSocketFD);

    return 0;
}