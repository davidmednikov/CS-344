/*
// #####################################################
// Program 4 - OTP Decoder Daemon
// David Mednikov
// CS344 Spring 2019
//
// otp_dec_d
//
// This program runs in the background as a daemon. The
// program will listen on a particular port/socket to be
// determined by the user as a runtime argument.
// The program will receive an encrypted string and key
// which will be used to decode the encrupted text into
// plaintext. The plaintext string will be returned
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
** decrypt Function
** ----------------------------------------------------
** function to decrypt encrypted text using a key
** params: string encryptedText, string key
** returns: string plaintext
** ----------------------------------------------------
*/
char* decrypt(char* encryptedText, char* key) {
    // allocate int array of same length as encryptedText to be decoded
    int* textNums = (int*) calloc(strlen(encryptedText), sizeof(int));

    // iterator variable
    int i;

    // loop through encryptedText string, copying ascii code to textNums array
    for (i = 0; i < strlen(encryptedText); i++) {
        // if space, add 26 to encryptedText array
        if (encryptedText[i] == 32) {
            textNums[i] = 26;
        } else {
            // not space, subtract 65 from ascii code so that 0-25 are letter codes
            textNums[i] = encryptedText[i] - 65;
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

    // allocate int array of same length as text to be decoded
    int* modulo = (int*) calloc(strlen(encryptedText), sizeof(int));

    // loop through array, subtract key values from encrypted values then modulo 27
    for (i = 0; i < strlen(encryptedText); i++) {
        modulo[i] = (textNums[i] - keyNums[i]);
        while (modulo[i] < 0) {
            modulo[i] += 27;
        }
        modulo[i] %= 27;
    }

    // allocate memory for decoded string
    char* decoded = (char*) calloc(strlen(encryptedText), sizeof(char));

    // loop through length of encryptedText and add decrypted char to decoded string
    for (i = 0; i < strlen(encryptedText); i++) {
        // get code of current char from modulo array
        int code = modulo[i];

        // if current char code represents a space, set to 32 (ascii code for space)
        if (code == 26) {
            decoded[i] = 32;
        } else {
            // not a space, add 65 to number to get ascii value
            decoded[i] = modulo[i] + 65;
        }
    }

    // return encrypted string to calling function
    return decoded;
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
** and decodes the encrpyted string using the provided key
** ----------------------------------------------------
*/
int main(int argc, char* argv[]) {
    // if incorrect number of arguments, display correct usage to user and quit
    if (argc != 2) {
        fprintf(stderr, "USAGE: otp_dec_d <listening_port>\n");
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
            char* ciphertext;
            char* key;

            // clear out buffer
            memset(buffer, '\0', 131072);

            // receive data from socket
            charsRead = recv(establishedConnectionFD, buffer, 131071, 0);
            if (charsRead < 0) {
                // error receiving data from socket
                error("SERVER: ERROR receiving data from socket\n");
            }

            // no error, make sure connection originates from otp_dec
            if (strcmp(buffer, "dec") == 0) {
                // connection is from otp_dec, clear out buffer and set to "dec_d" to send back to client
                memset(buffer, '\0', 131072);
                strcpy(buffer, "dec_d");

                // send buffer to client
                charsWritten = send(establishedConnectionFD, buffer, strlen(buffer), 0);
                if (charsWritten < 0) {
                    // error sending data to socket
                    error("SERVER: ERROR sending data to socket\n");
                }

                // no error, get length of ciphertext from client

                // clear buffer and receive ciphertext length from socket
                memset(buffer, '\0', 131072);
                charsRead = recv(establishedConnectionFD, buffer, 131071, 0);

                if (charsRead < 0) {
                    // error receiving data from socket
                    error("SERVER: ERROR receiving data from socket\n");
                }

                // no error, get length of ciphertext and return to client for verification
                int ciphertextLength = atoi(buffer);
                memset(buffer, '\0', 131072);
                sprintf(buffer, "%d", ciphertextLength);

                // send length back to client
                charsWritten = send(establishedConnectionFD, buffer, strlen(buffer), 0);
                if (charsWritten < 0) {
                    // error sending data to socket
                    error("SERVER: ERROR sending data to socket\n");
                }

                // variables to store ciphertext and keep track of received characters
                ciphertext = calloc(ciphertextLength + 1, sizeof(char));
                int charsReceived = 0;

                // while still receiving characters
                while (charsReceived < ciphertextLength) {
                    // clear out buffer and receive data from socket
                    memset(buffer, '\0', 131072);
                    charsRead = recv(establishedConnectionFD, buffer, 131071, 0);
                    if (charsRead < 0) {
                        // error receiving data from socket
                        error("SERVER: ERROR receiving data from socket\n");
                    }

                    // no error, cat buffer to ciphertext and increment chars received
                    strcat(ciphertext, buffer);
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

                // decrypt ciphertext and copy to buffer to send back to client
                memset(buffer, '\0', 131072);
                strcpy(buffer, decrypt(ciphertext, key));

                // send decrypted string back to client
                charsWritten = send(establishedConnectionFD, buffer, strlen(buffer), 0);
                if (charsWritten < 0) {
                    // error sending data to socket
                    error("SERVER: ERROR sending data to socket\n");
                }
            } else {
                // connection did not originate from otp_dec, ignore
                error("This connection did not originate from otp_dec");
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