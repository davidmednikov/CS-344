/*
// #####################################################
// Program 3 - Small Shell
// David Mednikov
// CS344 Spring 2019
//
// smallsh
//
// This program creates a shell interface for the user to
// interact with the operating system. This shell will support
// 3 built-in commands: cd, status, and exit. In addition, it
// will be able to execute bash commands and programs by
// spawning a child process and executing the command requested
// in the user's input.
// #####################################################
*/

// import everything
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

// define bools
typedef enum { false, true } bool;

/*
// CommandStruct struct
// ----------------------------------------------------
// struct to hold all attributes of a command entered by the user
// ----------------------------------------------------
*/
struct CommandStruct {
    char* command;
    char* args[512];
    int numArgs;
    char* inputFile;
    char* outputFile;
    bool backgrounded;
};

// array to hold child pids
int childProcesses[512];

// keep track of # of child processes
int numChildren = 0;

// store pid of most recent foreground process
int foregroundPid = -100;

// bool to keep track of foreground only mode
bool foregroundOnly = false;

// bool to keep track of active foreground tasks
bool foregroundActive = false;

/*
** stringStartsWith Function
** ----------------------------------------------------
** function to see if strings begins with a substring, to determine if line is a column
** params: full string, start of string
** returns: bool
** ----------------------------------------------------
*/
bool stringStartsWith(char* string, char* start) {
    return strncmp(string, start, strlen(start)) == 0 ? true : false;
}


/*
** trimWhiteSpace Function
** ----------------------------------------------------
** function to trim leading and trailing whitespace from a string
** params: string
** returns: string
** ----------------------------------------------------
*/
char* trimWhiteSpace(char* string)
{
    // string to capture trailing white soace
    char* trail;

    // Trim leading whitespace, increasing index until reaching non-whitespace
    while(isspace((unsigned char)*string)) {
        string++;
    }

    // entire string is spaces, return null terminator
    if(*string == 0) {
        return string;
    }

    // Trim trailing whitespace
    trail = string + strlen(string) - 1;

    // decrease index until reaching non-whitespace or string
    while(trail > string && isspace((unsigned char)*trail)) {
        trail--;
    }

    // Write new null terminator
    trail[1] = '\0';

    // return trimmed string
    return string;
}


/*
** expand$$ToPid Function
** ----------------------------------------------------
** function to expand instances of $$ into the pid of the calling function
** params: string
** returns: string
** ----------------------------------------------------
*/
char* expand$$ToPid(char* string) {
    // string where $$ was found
    char* found;

    // get pid of calling function
    pid_t shellPid = getpid();

    // copy pid into string
    char pidString[20];
    sprintf(pidString, "%d", shellPid);

    // if string contains $$, expand to pid
    if ((found = strstr(string, "$$")) != NULL) {
        // create string to hold return
        char* returnString = calloc(strlen(string) + 6, sizeof(char));

        // if $$ token is at start of string, copy pid to return string
        if (strlen(found) == strlen(string)) {
            strcpy(returnString, pidString);
        } else {
            // there is text before $$ token, copy text before token and then concatenate the pid
            strncpy(returnString, string, found - string);
            strcat(returnString, pidString);
        }

        // concatenate whatever came after the $$ token to the return string
        strcat(returnString, found + 2);

        // might be more than one instance of $$, call recursively
        return expand$$ToPid(returnString);
    }

    // no $$ in string, return string
    return string;
}


/*
** getCommand Function
** ----------------------------------------------------
** function to parse user input into a command with support for redirection & backgrounding
** params: none
** returns: struct CommandStruct*
** ----------------------------------------------------
*/
struct CommandStruct* getCommand() {
    // string to hold input
    char* input = calloc(2050, sizeof(char));

    // print colon prompt and flush stdout
    printf(": ");
    fflush(stdout);

    // get input from user
    fgets(input, 2050, stdin);

    // trim whitespace
    strcpy(input, trimWhiteSpace(input));

    // if user did not enter a comment or empty line, try parsing the input
    if (!stringStartsWith(input, "#") && input[0] != '\0') {
        // input exceeds max length, show error message and prompt again
        if (strlen(input) > 2048) {
            printf("Your command was longer than the allowed 2048 characters.\n");
            fflush(stdout);
        } else {
            // input is < 2048 characters

            // expand pid if present
            strcpy(input, expand$$ToPid(input));

            // allocate memory for command struct
            struct CommandStruct* commandStruct = (struct CommandStruct*) malloc(sizeof(struct CommandStruct));

            // pull command out from string
            char* command = strtok(input, " ");

            // allocate memory for command string and copy to command struct
            commandStruct->command = calloc(strlen(command), sizeof(char));
            strncpy(commandStruct->command, command, strlen(command));

            // initialize # of args to 0
            commandStruct->numArgs = 0;

            // loop through all args
            bool moreArgs = true;

            // if invalid input
            bool invalidInput = false;

            // while still reading input
            while (moreArgs) {
                // get next arg token
                char* arg = strtok(NULL, " \n");

                // if not end of line
                if (arg != NULL) {
                    // if arg is '<', next token is the input file
                    if (strcmp(arg, "<") == 0) {
                        char* inputFile = strtok(NULL, " ");

                        // allocate memory for input file and copy to command struct
                        commandStruct->inputFile = calloc(strlen(inputFile), sizeof(char));
                        strcpy(commandStruct->inputFile, inputFile);
                    } else if (strcmp(arg, ">") == 0) {
                        // arg is '>', next token is the output file
                        char* outputFile = strtok(NULL, " ");

                        // allocate memory for ouput file and copy to command struct
                        commandStruct->outputFile = calloc(strlen(outputFile), sizeof(char));
                        strcpy(commandStruct->outputFile, outputFile);
                    } else if (strcmp(arg, "&") == 0) {
                        // arg is '&', if more text after just treat is as text, if end of input background process
                        char* nextArg = strtok(NULL, " ");

                        // more text after &, treat as normal text and add to argument list
                        if (nextArg != NULL) {
                            // if there are currently 510 arguments or less, add & and next arg to list
                            if (commandStruct->numArgs < 511) {
                                // allocate memory for both & and next arg, and increment number of args twice
                                commandStruct->args[commandStruct->numArgs] = calloc(strlen(arg), sizeof(char));
                                strcpy(commandStruct->args[commandStruct->numArgs], arg);
                                commandStruct->numArgs++;

                                commandStruct->args[commandStruct->numArgs] = calloc(strlen(nextArg), sizeof(char));
                                strcpy(commandStruct->args[commandStruct->numArgs], nextArg);
                                commandStruct->numArgs++;
                            } else {
                                // too many args
                                invalidInput = true;
                            }
                        } else {
                            // reached end of line, if backgrounding allowed, set backgrounded to true
                            if (!foregroundOnly) {
                                commandStruct->backgrounded = true;
                            }
                            // break out of loop
                            moreArgs = false;
                        }
                    } else {
                        // not input, output, or backgrounding character, add to args list if < 512 args
                        if (commandStruct->numArgs < 512) {
                            // space for more args, allocate memory for arg, copy to list, and increment numArgs
                            commandStruct->args[commandStruct->numArgs] = calloc(strlen(arg), sizeof(char));
                            strcpy(commandStruct->args[commandStruct->numArgs], arg);
                            commandStruct->numArgs++;
                        } else {
                            // too many args
                            invalidInput = true;
                        }
                    }
                } else {
                    // reached end of line, break out of loop
                    moreArgs = false;
                }
                if (invalidInput) {
                    // invalid input, break out of loop
                    moreArgs = false;
                }
            }
            // if too many args, print error
            if (invalidInput) {
                printf("Your command exceeded the allowed 512 arguments.\n");
                fflush(stdout);
            } else {
                free(input);

                // valid input, return commandStruct to caller
                return commandStruct;
            }
        }
    }
    free(input);

    // bad input so return NULL
    return NULL;
}


/*
** toggleForegroundOnly Function
** ----------------------------------------------------
** function to toggle foreground only mode on or off
** params: none
** returns: none
** ----------------------------------------------------
*/
void toggleForegroundOnly() {
    // wait for foreground to finish
    int exitStatus;
    waitpid(foregroundPid, &exitStatus, 0);

    // if in foreground only mode, exit foreground-only mode
    if (foregroundOnly) {
        char* message = "Exiting foreground-only mode\n";
        write(STDOUT_FILENO, message, 30);
        fflush(stdout);
    } else {
        // not in foreground only mode, enter foreground-only mode
        char* message = "Entering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, 50);
        fflush(stdout);
    }

    // toggle foreground only flag
    foregroundOnly = !foregroundOnly;
}


/*
** changeDir Function
** ----------------------------------------------------
** function to change the working directory - recreate cd functionality
** params: commandStruct*
** returns: none
** ----------------------------------------------------
*/
void changeDir(struct CommandStruct* commandStruct) {
    // if args, change directory to provided argument
    if (commandStruct->numArgs > 0) {
        chdir(commandStruct->args[0]);
    } else {
        // no args, change directory to home directory
        chdir(getenv("HOME"));
    }
}


/*
** getStatus Function
** ----------------------------------------------------
** function to get the status of last killed function
** params: int childStatus
** returns: none
** ----------------------------------------------------
*/
void getStatus(int childStatus) {
    // if a foreground process has been run
    if (foregroundPid != -100) {
        // if process exited, print exit value
        if (WIFEXITED(childStatus)) {
            printf("exit value %d\n", WEXITSTATUS(childStatus));
            fflush(stdout);
        } else if (WIFSIGNALED(childStatus)) {
            // if process killed by signal, print info about signal
            printf("terminated by signal %d\n", WTERMSIG(childStatus));
            fflush(stdout);
    }
    } else {
        // no foreground process has run yet, return 0
        printf("exit value %d\n", 0);
    }
}


/*
** exitShell Function
** ----------------------------------------------------
** function to kill all child processes and exit shell
** params: none
** returns: none
** ----------------------------------------------------
*/
void exitShell() {
    int i;
    // loop through all child processes and send SIGKILL to them
    for (i = 0; i < numChildren; i++) {
        kill(childProcesses[i], SIGKILL);
    }
}


/*
** addToPidArray Function
** ----------------------------------------------------
** function to add a pid to the pid array and increment number of children
** params: int pid
** returns: none
** ----------------------------------------------------
*/
void addToPidArray(int pid) {
    childProcesses[numChildren] = pid;
    numChildren++;
}


/*
** removeFromPidArray Function
** ----------------------------------------------------
** function to remove a pid from the pid array and decrement number of children
** params: int pid
** returns: none
** ----------------------------------------------------
*/
void removeFromPidArray(int pid) {
    // iterator and foundIndex
    int i;
    int foundIndex = -1;

    // loop through children and search for matching pid
    for (i = 0; i < numChildren; i++) {
        if (childProcesses[i] == pid) {
            foundIndex = i;
        }
    }

    // if matching pid was found
    if (foundIndex != -1) {
        // decrement number of children
        numChildren--;

        // remove pid from array and shift all remaining elements to the left
        for (i = foundIndex; i < numChildren; i++) {
            childProcesses[i] = childProcesses[i+1];
        }
    }
}


/*
** deleteStruct Function
** ----------------------------------------------------
** function to free all memory in a command struct and set the pointer to null
** params: struct CommandStruct* commandStruct
** returns: none
** ----------------------------------------------------
*/
void deleteStruct(struct CommandStruct* commandStruct) {
    // loop through args and free them all
    int i;
    for (i = 0; i < commandStruct->numArgs; i++) {
        free(commandStruct->args[i]);
    }

    // free the actual command
    free(commandStruct->command);

    // if input and/out output files not null, free them
    if (commandStruct->inputFile != NULL) {
        free(commandStruct->inputFile);
    }
    if (commandStruct->outputFile != NULL) {
        free(commandStruct->outputFile);
    }

    // reset pointer to NULL
    commandStruct = NULL;
}


/*
** startShell Function
** ----------------------------------------------------
** start the shell and prompt user for commands until they exit
** params: int pid
** returns: none
** ----------------------------------------------------
*/
void startShell() {
    // keep running until exit
    bool active = true;

    // set up SIGINT handler to ignore signal and print info about killed child process
    struct sigaction SIGINT_act = {0};
    SIGINT_act.sa_handler = SIG_IGN;
    sigaction(SIGINT, &SIGINT_act, NULL);

    struct sigaction SIGTSTP_act = {0};
    SIGTSTP_act.sa_handler = toggleForegroundOnly;
    sigaction(SIGTSTP, &SIGTSTP_act, NULL);

    // initialize childPid and finishedStatus
    pid_t childPid = -420;
    int finishedStatus = -69;

    // while user has not exited
    while(active) {
        // get command from user
        struct CommandStruct* commandStruct = getCommand();

        // if valid input, run user command
        if (commandStruct != NULL) {
            // if user entered cd, call built-in cd function
            if (strcmp(commandStruct->command, "cd") == 0) {
                changeDir(commandStruct);
            } else if (strcmp(commandStruct->command, "status") == 0) {
                // user entered status, call built-in status function
                getStatus(finishedStatus);
            } else if (strcmp(commandStruct->command, "exit") == 0) {
                // user entered exit, call built-in exit function to kill all children and break out of loop
                exitShell();
                active = false;
            } else {
                // user entered non built-in function, create new arg array to hold # of args + 2 (for command name + NULL)
                char* execArgs[commandStruct->numArgs + 2];

                // allocate memory for command name and copy to 0 index
                execArgs[0] = calloc(strlen(commandStruct->command), sizeof(char));
                strcpy(execArgs[0], commandStruct->command);

                // iterate through args array, copying each to execArgs
                int i;
                for (i = 0; i < commandStruct->numArgs; i++) {
                    execArgs[i+1] = calloc(strlen(commandStruct->args[i]), sizeof(char));
                    strcpy(execArgs[i+1], commandStruct->args[i]);
                }

                // last arg in execArgs should be NULL
                execArgs[commandStruct->numArgs + 1] = NULL;

                // fork off child process
                childPid = fork();
                // in child process, perform any necessary redirections
                if (childPid == 0) {
                    SIGTSTP_act.sa_handler = SIG_IGN;
                    sigaction(SIGTSTP, &SIGTSTP_act, NULL);

                    // if backgrounded, redirect input and output to /dev/null unless otherwise specified
                    if (commandStruct->backgrounded) {
                        // if no specified input redirection, redirect to /dev/null dump
                        if (commandStruct->inputFile == NULL) {
                            int devNullDirRead = open("/dev/null", O_RDONLY);
                            dup2(devNullDirRead, 0);
                        }

                        // if no specified output redirection, redirect to /dev/null dump
                        if (commandStruct->outputFile == NULL) {
                            int devNullDirWrite = open("/dev/null", O_WRONLY);
                            dup2(devNullDirWrite, 1);
                        }
                    } else {
                        // foreground process, set signal handler to default behavior on SIGINT
                        SIGINT_act.sa_handler = SIG_DFL;
                        sigaction(SIGINT, &SIGINT_act, NULL);
                    }

                    // input file specified, redirect stdin to file
                    if (commandStruct->inputFile != NULL) {
                        // try opening file
                        int inputFileDescriptor = open(commandStruct->inputFile, O_RDONLY);
                        if (inputFileDescriptor == -1) {
                            // error opening file, print error and exit
                            fprintf(stderr, "Error opening file %s for reading\n", commandStruct->inputFile);
                            fflush(stdout);
                            exit(1);
                        }

                        // try redirecting input
                        int dupResult = dup2(inputFileDescriptor, 0);
                        if (dupResult == -1) {
                            // error redirecting input, print error and exit
                            fprintf(stderr, "Error redirecting stdin to %s\n", commandStruct->inputFile);
                            fflush(stdout);
                            exit(1);
                        }
                    }

                    // output file specified, redirect stdout to file
                    if (commandStruct->outputFile != NULL) {
                        // try creating or opening file
                        int outputFileDescriptor = open(commandStruct->outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (outputFileDescriptor == -1) {
                            // error opening file, print error and exit
                            fprintf(stderr, "Error creating file %s for writing\n", commandStruct->outputFile);
                            fflush(stdout);
                            exit(1);
                        }

                        // try redirecting output
                        int dupResult = dup2(outputFileDescriptor, 1);
                        if (dupResult == -1) {
                            // error redirecting output, print error and exit
                            fprintf(stderr, "Error redirecting stdout to %s\n", commandStruct->outputFile);
                            fflush(stdout);
                            exit(1);
                        }
                    }

                    // exec function
                    char command[2050];
                    strcpy(command, commandStruct->command);
                    deleteStruct(commandStruct);
                    int error = execvp(command, execArgs);
                    if (error == -1) {
                        // if error, print error and exit
                        fprintf(stderr, "%s: no such file or directory\n", command);
                        fflush(stdout);
                        exit(1);
                    } else {
                        exit(-1);
                    }
                } else {
                    // parent process

                    // if backgrounded, print pid of backgrounded function
                    if (commandStruct->backgrounded) {
                        printf("background pid is %d\n", childPid);
                        fflush(stdout);

                        // add pid to array
                        addToPidArray(childPid);
                    } else {
                        // set firegroundPid global variable
                        foregroundPid = childPid;
                        // not backgrounded, wait for process to finish and store exit status in finishedStatus
                        waitpid(childPid, &finishedStatus, 0);
                        if (WIFSIGNALED(finishedStatus)) {
                            printf("terminated by signal %d\n", WTERMSIG(finishedStatus));
                            fflush(stdout);
                        }
                    }
                    deleteStruct(commandStruct);
                }
            }
        }

        // before returning control to the user, check if any backgrounded processes finished
        int bgStatus;
        pid_t finishedProcess = waitpid(-1, &bgStatus, WNOHANG);

        // if process finished, remove from array and print info about how it exited
        if (finishedProcess > 0 && active) {
            // if backgrounded, remove pid from process array
            removeFromPidArray(finishedProcess);

            // if process exited
            if (WIFEXITED(bgStatus)) {
                printf("background pid %d is done: exit value %d\n", finishedProcess, WEXITSTATUS(bgStatus));
                fflush(stdout);
            } else {
                // if process was terminated by a signal
                printf("background pid %d is done: terminated by signal %d\n", finishedProcess, WTERMSIG(bgStatus));
                fflush(stdout);
            }
        }
    }
}


/*
** main Function
** ----------------------------------------------------
** run the shell
** params: none
** returns: int
** ----------------------------------------------------
*/
int main() {
    startShell();
    return 0;
}
