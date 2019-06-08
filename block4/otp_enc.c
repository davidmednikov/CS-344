/*
// #####################################################
// Program 4 - OTP Encoder Client
// David Mednikov
// CS344 Spring 2019
//
// otp_enc
//
// This program takes a plaintext file and encryption key
// as runtime arguments and sends them to a server for
// encryption. After getting the encrypted string back,
// it is printed to stdout.
// #####################################################
*/

// import everything
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/*
** error Function
** ----------------------------------------------------
** function to print a string to stderr
** params: string
** returns: none
** ----------------------------------------------------
*/
void error(const char* error) {
    fprintf(stderr, "%s", error);
}

/*
** checkIfKeyTooShort Function
** ----------------------------------------------------
** function to check if the key is shorter than the plaintext
** throws an error and exits if key is too short
** params: string plaintext, string key, string keyName
** returns: void
** ----------------------------------------------------
*/
void checkIfKeyTooShort(char* plaintext, char* key, char* keyFile) {
    // if length of key is less than length of plaintext, print error and exit
    if (strlen(key) < strlen(plaintext)) {
        fprintf(stderr, "otp_enc: ERROR key '%s' is too short\n", keyFile);
        exit(1);
    }
}

/*
** checkIfContainsBadText Function
** ----------------------------------------------------
** function to check if the plaintext or key contain any
** characters that are considered "bad" input. Any character
** other than capital A-Z or space is considered "bad" data
** params: string plaintext, string key, string plaintextFile, string keyFile
** returns: void
** ----------------------------------------------------
*/
void checkIfContainsBadText(char* plaintext, char* key, char* plaintextFile, char* keyFile) {
    // iterator variable
    int i;

    // loop through plaintext to see if it contains bad input
    for (i = 0; i < strlen(plaintext); i++) {
        // if any character is not A-Z or a space, it is bad input
        if (plaintext[i] != 32 && (plaintext[i] < 65 || plaintext[i] > 90)) {
            fprintf(stderr, "otp_enc: ERROR plaintext '%s' contains bad characters\n", plaintextFile);
            exit(1);
        }
    }

    // loop through key to see if it contains bad input
    for (i = 0; i < strlen(key); i++) {
        // if any character is not A-Z or a space, it is bad input
        if (key[i] != 32 && (key[i] < 65 || key[i] > 90)) {
            fprintf(stderr, "otp_enc: ERROR key '%s' contains bad characters\n", keyFile);
            exit(1);
        }
    }
}

/*
** inputIsValid Function
** ----------------------------------------------------
** function to verify that the provided plaintext and key
** files are valid according to program specifications
** params: string plaintext, string key
** returns: true if valid, false if not
** ----------------------------------------------------
*/
void inputIsValid(char* plaintext, char* key, char* plaintextFile, char* keyFile) {
    // if key is shorter than plaintext, display error and exit
    checkIfKeyTooShort(plaintext, key, keyFile);
    // if bad input, display error and exit
    checkIfContainsBadText(plaintext, key, plaintextFile, keyFile);
}

/*
** main
** ----------------------------------------------------
** main function that creates the socket and attempts
** to connect to the server.
** ----------------------------------------------------
*/
int main (int argc, char* argv[]) {
    // if arg count is wrong, display message showing correct usage to user
    if (argc != 4) {
        error("USAGE: otp_enc <plaintext_file> <key> <port_number>\n");
        exit(1);
    }

    // initialize variables and structs
    int portNumber;
    int socketFD;
    int charsWritten;
    int charsRead;

    struct sockaddr_in serverAddress;

    char buffer[131072];

    FILE* plaintextFile;
    FILE* keyFile;

    portNumber = atoi(argv[3]);

    // try opening plaintext file
    plaintextFile = fopen(argv[1], "r");
    if (plaintextFile != NULL) {
        // plaintext file opened successfully, try to open key file
        keyFile = fopen(argv[2], "r");
        if (keyFile != NULL) {
            // key file opened successfully, send plaintext and key to server

            // clear out memory in server address struct
            memset((char*) &serverAddress, '\0', sizeof(serverAddress));

            // set serverAddress properties per video lecture
            serverAddress.sin_family = AF_INET;
            serverAddress.sin_port = htons(portNumber);

            // open socket and print error if failed
            socketFD = socket(AF_INET, SOCK_STREAM, 0);
            if (socketFD < 0) {
                // error encountered while opening socket
                fprintf(stderr, "otp_enc: ERROR opening socket with port %d\n", portNumber);
                exit(2);
            }

            // connect to socket and print error if failed
            if (connect(socketFD, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0) {
                // error encountered while connecting to socket
                fprintf(stderr, "otp_enc: ERROR connecting with port %d\n", portNumber);
                exit(2);
            }

            // Socket is connected, clear out buffer array and set to "enc" to identify client to server
            memset(buffer, '\0', 131072);
            strcpy(buffer, "enc");

            // send buffer to socket
            charsWritten = send(socketFD, buffer, strlen(buffer), 0);

            if (charsWritten < 0) {
                // error writing to socket
                error("otp_enc: ERROR sending to socket\n");
            }

            // clear buffer after sending
            memset(buffer, '\0', 131072);

            // receive data from socket
            charsRead = recv(socketFD, buffer, 131071, 0);
            if (charsRead < 0) {
                // error receiving from socket
                error("otp_enc: ERROR receiving from socket\n");
            }

            // make sure that server identified itself as "otp_enc_d", reject if not
            if (strcmp(buffer, "enc_d") == 0) {
                // create strings to store file contents
                char plaintext[131072];
                fgets(plaintext, 131072, plaintextFile);

                char key[131072];
                fgets(key, 131072, keyFile);

                // strip newlines
                plaintext[strcspn(plaintext, "\n")] = '\0';
                key[strcspn(key, "\n")] = '\0';

                // validate inputs
                checkIfKeyTooShort(plaintext, key, argv[2]);
                checkIfContainsBadText(plaintext, key, argv[1], argv[2]);

                // clear buffer memory and set to length of plaintext
                memset(buffer, '\0', 131072);
                sprintf(buffer, "%zd", strlen(plaintext));

                // send length of plaintext to server, server will verify
                charsWritten = send(socketFD, buffer, strlen(buffer), 0);
                if (charsWritten < 0) {
                    error("otp_enc: ERROR sending to socket\n");
                }

                // clear buffer memory
                memset(buffer, '\0', 131072);

                // receive data from socket
                charsRead = recv(socketFD, buffer, 131071, 0);

                // no characters read, so there was an error receiving
                if (charsRead < 0) {
                    error("otp_enc: ERROR receiving from socket\n");
                }

                // create string to store plaintext length for string comparison purposes
                char plaintextLength[256];
                memset(plaintextLength, '\0', 256);
                sprintf(plaintextLength, "%zd", strlen(plaintext));

                if (strcmp(buffer, plaintextLength) != 0) {
                    // server did not send back correct plaintext length, throw error
                    error("otp_enc: ERROR otp_enc_d did not acknowledge sent message\n");
                }

                // clear buffer memory and copy plaintext into it
                memset(buffer, '\0', 131072);
                strcpy(buffer, plaintext);

                // send plaintext to server
                charsWritten = send(socketFD, buffer, strlen(buffer), 0);
                if (charsWritten < 0) {
                    // error sending to socket
                    error("otp_enc: ERROR sending to socket\n");
                }

                // receive data from socket
                memset(buffer, '\0', 131072);
                charsRead = recv(socketFD, buffer, 131071, 0);
                if (charsRead < 0) {
                    // error receiving from socket
                    error("otp_enc: ERROR receiving from socket\n");
                }

                // verify that server got entire plaintext contents
                if (strcmp(buffer, plaintextLength) != 0) {
                    error("otp_enc: ERROR otp_enc_d did not receive entire message\n");
                }

                // clear buffer memory and copy key length into it
                memset(buffer, '\0', 131072);
                sprintf(buffer, "%zd", strlen(key));

                // send key length to server
                charsWritten = send(socketFD, buffer, strlen(buffer), 0);
                if (charsWritten < 0) {
                    error("otp_enc: ERROR sending to socket\n");
                }

                // clear buffer and receive data from socket
                memset(buffer, '\0', 131072);
                charsRead = recv(socketFD, buffer, 131071, 0);
                if (charsRead < 0) {
                    error("otp_enc: ERROR receiving from socket\n");
                }

                // get key length for verification purposes
                char keyLength[256];
                memset(keyLength, '\0', 256);
                sprintf(keyLength, "%zd", strlen(key));

                // verify that server got the key length
                if (strcmp(buffer, keyLength) != 0) {
                    error("otp_enc: ERROR otp_enc_d did not acknowledge sent message\n");
                }

                // clear buffer memory and copy key into it
                memset(buffer, '\0', 131072);
                strcpy(buffer, key);

                // send key to server
                charsWritten = send(socketFD, buffer, strlen(buffer), 0);
                if (charsWritten < 0) {
                    error("otp_enc: ERROR sending to socket\n");
                }

                // clear buffer and receive data from socket
                memset(buffer, '\0', 131072);
                charsRead = recv(socketFD, buffer, 131071, 0);
                if (charsRead < 0) {
                    error("otp_enc: ERROR receiving from socket\n");
                }

                // verify that server received entire contents
                if (strcmp(buffer, keyLength) != 0) {
                    error("otp_enc: ERROR otp_enc_d did not receive entire message\n");
                }

                // allocate memory for ciphertext string
                char* ciphertext = (char*) calloc(strlen(plaintext), sizeof(char));

                // loop until entire message received
                int charsReceived = 0;
                while (charsReceived < strlen(plaintext)) {
                    // clear out buffer and receive data from socket
                    memset(buffer, '\0', 131072);
                    charsRead = recv(socketFD, buffer, 131071, 0);
                    if (charsRead < 0) {
                        // error receiving data from socket
                        error("otp_enc: ERROR receiving data from socket\n");
                    }

                    // no error, cat buffer to ciphertext and increment chars received
                    strcat(ciphertext, buffer);
                    charsReceived += strlen(buffer);
                }

                // send ciphertext string to stdout
                printf("%s\n", ciphertext);
            } else {
                // connected to server other than otp_enc_d, throw error and quit
                fprintf(stderr, "otp_enc: ERROR port %d connection is not to otp_enc_d, closing\n", portNumber);
                exit(2);
            }

            // close socket
            close (socketFD);
        } else {
            // error opening key file
            fprintf(stderr, "otp_enc: ERROR opening key file %s\n", argv[2]);
            exit(1);
        }
    } else {
        // error opening plaintext file
        fprintf(stderr, "otp_enc: ERROR opening plaintext file %s\n", argv[1]);
        exit(1);
    }

    // return 0 and exit
    return 0;
}
