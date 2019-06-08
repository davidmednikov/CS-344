/*
// #####################################################
// Program 4 - OTP Keygen
// David Mednikov
// CS344 Spring 2019
//
// keygen
//
// This program creates an encryption key with user-specified length.
// The key will consist of all uppercase letters and spaces only.
// The key will be printed to stdout, however output can
// be redirected so that the key is saved to a file instead.
// #####################################################
*/

// import everything
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main (int argc, char *argv[]) {
    // seed random using time
    srand(time(NULL));

    // int to store length of key
    int length;

    // if arg count is incorrect, display message showing how to correctly call keygen program
    if (argc != 2) {
        fprintf(stderr, "USAGE: keygen <key_length>\n");
        return 1;
    } else {
        // arg count is correct, get length from argument and generate key randomly
        length = atoi(argv[1]);

        // create string to hold key of specified length + 1 (for newline)
        char* key = (char*) calloc(length + 1, sizeof(char));

        // loop that runs once for each character in the key of user-specified length
        int i;
        for (i = 0; i < length; i++) {
            // generate random number between 0 and 26
            int random = rand() % 27;

            // var to hold next character to be added to key
            char nextChar;

            // if random generates a 0, treat that as a space
            if (random == 0) {
                nextChar = 32;
            } else {
                // if random generates non-0, add 64 to get the associated ASCII letter.
                nextChar = random + 64;
            }

            // add char to current index of key
            key[i] = nextChar;
        }

        // append newline to end of key
        key[length] = '\n';

        // print key to stdout, can be redirected to another file
        printf("%s", key);

        return 0;
    }
}
