/*
// #####################################################
// Program 2 - Adventure
// David Mednikov
// CS344 Spring 2019
//
// game program
//
// This program randomly finds the newest set of room
// files in a directory and reads them to create the game
// graph. The program asks the user which room they would
// like to go to until the user reaches the END_ROOM.
// At this point the game ends and tells the user about
// their long, treacherous journey to the END_ROOM.
// #####################################################
*/

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* define boolean enums */
typedef enum { false, true } bool;

/*
// Room struct
// ----------------------------------------------------
// struct to hold all attributes of adventure room
// ----------------------------------------------------
*/
struct Room {
    int id;
    char name[9];
    char roomType[11];
    int numberOutboundConnections;
    struct Room* outboundConnections[6];
    char* connectionNames[6];
};

/* static lists of Rooms and room names */
static struct Room* roomsList[7];

pthread_mutex_t timeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t timeThread;

/*
** getNewDirectoryName Function
** ----------------------------------------------------
** function to return the name of the newest directory
** params: none
** returns: string
** ----------------------------------------------------
*/
char* getNewDirectoryName() {
    /* initialize newest time to -1 */
    int newDirTime = -1;

    /* search for directory names beginning with 'mednikod.rooms.' */
    char dirPrefix[32] = "mednikod.rooms.";

    /* initialize newest directory name */
    static char newDirName[128];
    memset(newDirName, '\0', sizeof(newDirName));

    /* create pointers to directory and files inside */
    DIR* readDir;
    struct dirent* readFile;

    /* create directory attributes stat struct */
    struct stat dirAttr;

    /* open current directory to read subdirectories named 'mednikod.rooms.#####' */
    readDir = opendir(".");

    if (readDir > 0) {
        /* directory opened successfully, loop through all subdirectories */
        while ((readFile = readdir(readDir)) != NULL) {
            /* if file/folder begins with prefix */
            if(strstr(readFile->d_name, dirPrefix) != NULL) {
                /* get stats of file/folder and store to attributes */
                stat(readFile->d_name, &dirAttr);

                /* if current directory's modified time is greater than the running newest modified time */
                if ( (int) dirAttr.st_mtime > newDirTime) {
                    /* update newest modified time to current directory's modified time */
                    newDirTime = (int) dirAttr.st_mtime;

                    /* set current directory name as newest directory name */
                    memset(newDirName, '\0', sizeof(newDirName));
                    strcpy(newDirName, readFile->d_name);
                 }
            }
        }
    }
    /* close directory */
    closedir(readDir);

    /* return name of newest directory */
    return newDirName;
}


/*
** readRooms Function
** ----------------------------------------------------
** function to read rooms and add them to global rooms list
** params: name of newest directory
** returns: void
** ----------------------------------------------------
*/
void readRooms(char* newestDir) {
    /* initialize variables for name and iterator */
    char fileName[64];
    int i;

    /* read 7 room files */
    for (i = 0; i < 7; i++) {
        /* allocate memory for room */
        struct Room* room = (struct Room*) malloc(sizeof(struct Room));

        /* create filename and open file */
        sprintf(fileName, "./%s/file_%d", newestDir, i);
        FILE* file = fopen(fileName, "r");

        /* if still reading files */
        if (file != NULL) {
            /* set id and number of connections */
            room->id = i;
            room->numberOutboundConnections = 0;

            /* read until reaching end of file */
            bool keepReading = true;
            while (keepReading) {
                /* get current line from file */
                char line[64];
                fgets(line, 64, file);

                /* if line begins with ROOM_NAME, get name */
                if (strstr(line, "ROOM NAME: ") != NULL) {
                    sscanf(line, "ROOM NAME: %s\n", room->name);
                } else if (strstr(line, "ROOM TYPE: ") != NULL) {
                    /* else if line begins with ROOM_TYPE, get type */
                    sscanf(line, "ROOM TYPE: %s\n", room->roomType);
                    keepReading = false;
                } else {
                    /* else line lists a connection, get name of connecting room */
                    char name[10];
                    sscanf(line, "CONNECTION %*d: %s\n", name);

                    /* allocate memory and store name of room in connection names array */
                    room->connectionNames[room->numberOutboundConnections] = calloc(10, sizeof(char));
                    strcpy(room->connectionNames[room->numberOutboundConnections], name);

                    /* increment number of outbound connections */
                    room->numberOutboundConnections++;
                }
            }
            /* add room to static rooms list */
            roomsList[i] = room;
        }
    }
}


/*
** getRoomId Function
** ----------------------------------------------------
** function to return room id when given a room name
** params: room name
** returns: int room id, -1 if not found
** ----------------------------------------------------
*/
int getRoomId(char* name) {
    /* loop through 7 rooms */
    int i;
    for (i = 0; i < 7; i++) {
        /* if search name matches name of room, return room id (also room index) */
        if (strstr(roomsList[i]->name, name) != NULL) {
            return i;
        }
    }

    /* if user entered 'time', return an id of -1 */
    if (strstr(name, "time") != NULL && strlen(name) == 4) {
        return -1;
    }

    /* room name not found, return -2 */
    return -2;
}


/*
** setConnections Function
** ----------------------------------------------------
** function to connect rooms by reading connection names
** and initializing pointers to corresponding rooms
** in each room's outboundConnections array
** params: none
** returns: void
** ----------------------------------------------------
*/
void setConnections() {
    /* loop through each room */
    int i;
    for (i = 0; i < 7; i++) {
        /* second iterator to loop through room's outbound connections */
        int j;
        for (j = 0; j < roomsList[i]->numberOutboundConnections; j++) {
            /* get corresponding room id of name i connection names */
            int roomId = getRoomId(roomsList[i]->connectionNames[j]);
            roomsList[i]->outboundConnections[j] = roomsList[roomId];
        }
    }
}


/*
** printRound Function
** ----------------------------------------------------
** function to print current room and available
** connecting rooms
** params: int roomId
** returns: void
** ----------------------------------------------------
*/
void printRound(int roomId) {
    /* get room from roomId */
    struct Room* room = roomsList[roomId];

    /* print name of current room */
    printf("CURRENT LOCATION: %s\n", room->name);

    /* initialize string to print connections */
    char connectionsString[128];

    /* loop through connections */
    int i;
    for (i = 0; i < room->numberOutboundConnections; i++) {
        /* if first connection, set contents of connections string to connection name */
        if (i == 0) {
            strcpy(connectionsString, room->outboundConnections[i]->name);
        } else {
            /* else not the first connection, append name to connections string */
            strcat(connectionsString, room->outboundConnections[i]->name);
        }

        /* if all connections in string append a period */
        if (i == room->numberOutboundConnections - 1) {
            strcat(connectionsString, ".");
        } else {
            /* still listing connections so append a comma */
            strcat(connectionsString, ", ");
        }
    }
    /* print connections string */
    printf("POSSIBLE CONNECTIONS: %s\n", connectionsString);
}


/*
** getTime Function
** ----------------------------------------------------
** threaded function that gets the current time and writes it to a file
** ----------------------------------------------------
*/
void* getTime(void * arg) {
    /* get current unix time */
    time_t seconds = time(NULL);

    /* string to hold time */
    char timeString[40];

    /* format time per spec and save to string */
    strftime(timeString, 40, "%l:%M%P, %A, %B %d, %Y", localtime(&seconds));

    /* try to lock mutex */
    int resultCode = pthread_mutex_lock(&timeMutex);
    assert(resultCode == 0);

    /* mutex locked, open file for writing and write time string to file */
    FILE* file = fopen("currentTime.txt", "w");
    fprintf(file, "%s", timeString);
    fclose(file);

    /* unlock mutex */
    pthread_mutex_unlock(&timeMutex);

    return NULL;
}


/*
** getNextRoom Function
** ----------------------------------------------------
** function to get next room choice from user
** params: int currentRoom
** returns: int roomId
** ----------------------------------------------------
*/
int getNextRoom(int currentRoom) {
    /* initialize variable for user input */
    char input[32];
    while (true) {
        /* print query and get user's input */
        printf("WHERE TO? >");

        /* remove white space and copy to name */
        char name[32];

        /* get roomId of passed in name */
        int roomId = -1;

        /* if user entered 'time' */
        if (roomId == -1) {
            /* unlock the mutex and stop running until the time thread terminates */
            pthread_mutex_unlock(&timeMutex);
            int joinResultCode = pthread_join(timeThread, NULL);
            assert(joinResultCode == 0);

            /* open file written by time thread for reading */
            FILE* timeFile = fopen("currentTime.txt", "r");
            if (timeFile != NULL) {
                /* file opened successfully, get first line and print to output */
                char timeLine[40];
                fgets(timeLine, 40, timeFile);
                printf("\n%s\n\n", timeLine);

                /* close file */
                fclose(timeFile);
            }

            /* lock the mutex and re-create a new time thread since the last one terminated */
            int lockResult = pthread_mutex_lock(&timeMutex);
            assert(lockResult == 0);
            int newThreadResultCode = pthread_create(&timeThread, NULL, getTime, NULL);
            assert(newThreadResultCode == 0);

            /* time doesn't count as a step, so show rooms again and prompt for input */
            printRound(currentRoom);
        } else if (roomId == -2 || strlen(name) != strlen(roomsList[roomId]->name)) {
            /* if room was not found or passed-in name was not the same length as the room name,
               print error and show room prompt again, then go back to top of loop */
            printf("\nHUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
            printRound(currentRoom);
        } else {
            /* room was found but need to make sure it's one of the ones where a connection exists */
            bool hasConnection = false;

            /* loop through outbound connections list */
            int i;
            for (i = 0; i < roomsList[currentRoom]->numberOutboundConnections; i++) {
                /* if roomId of user-selected room is found in outboundConnections */
                if (roomId == roomsList[currentRoom]->outboundConnections[i]->id) {
                    /* set bool to true */
                    hasConnection = true;
                }
            }

            /* if connection does not exist, print error and loop back to top */
            if (!hasConnection) {
                printf("\nHUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
                printRound(currentRoom);
            } else {
                /* connection exists, print a newline and return the roomId of the next room */
                printf("\n");
                return roomId;
            }
        }
    }

    /* shouldn't get here, but returning -1 means something wrong happened */
    return -1;
}


/*
** playGame Function
** ----------------------------------------------------
** function to play the game
** params: none
** returns: void
** ----------------------------------------------------
*/
void playGame() {
    pthread_mutex_lock(&timeMutex);

    int resultCode = pthread_create(&timeThread, NULL, getTime, NULL);
    assert(resultCode == 0);

    /* bool to keep game going until user selects end room */
    bool stillPlaying = true;

    /* store roomId of current room */
    int roomId = 0;

    /* steps counter */
    int steps = 0;

    /* array to hold path taken */
    char* path[100];

    /* keep looping until player selects end room */
    while (stillPlaying) {
        /* print current room and connections then get users's choice of next room */
        printRound(roomId);
        roomId = getNextRoom(roomId);

        /* allocate memory for the path and add to the path array */
        path[steps] = calloc(10, sizeof(char));
        strcpy(path[steps], roomsList[roomId]->name);

        /* incrememnt steps counter */
        steps++;

        /* if current room is an END_ROOM, stop looping */
        if (strstr(roomsList[roomId]->roomType, "END_ROOM") != NULL) {
            stillPlaying = false;
        }
    }

    /* print end message and # of steps */
    printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", steps);

    /* print the path taken */
    int i;
    for (i = 0; i < steps; i++) {
        printf("%s\n", path[i]);
    }

    pthread_mutex_destroy(&timeMutex);
}


/*
** main Function
** ----------------------------------------------------
** finds the newest directory, reads the rooms files inside,
** connects the rooms to each other for the game, and starts
** the game
** params: none
** returns: void
** ----------------------------------------------------
*/
int main() {
    /* save newest directory name in memory */
    char newDirName[128];
    strcpy(newDirName, getNewDirectoryName());

    /* read rooms inside newest directory */
    readRooms(newDirName);

    /* set up connectiomns between rooms */
    setConnections();

    /* play the game! */
    playGame();

    return 0;
}