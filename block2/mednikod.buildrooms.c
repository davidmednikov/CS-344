/*
// #####################################################
// Program 2 - Adventure
// David Mednikov
// CS344 Spring 2019
//
// rooms program
//
// This program randomly selects 7 names from a hard-coded
// list of 10 names. The program creates 7 rooms, giving
// each room one of the randomly selected names. The rooms
// are connected to each other until the graph is full. The
// program then writes the details about each room to a new file
// to be used by the game.
// #####################################################
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <unistd.h>

/* define boolean enums */
typedef enum { false, true } bool;

/* hard-code room names */
char names[10][9] = { "Assembly", "Crystal", "Elixir", "Mercury", "Perl", "Python", "React", "Ring", "Ruby", "SPARK" };

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
};

/*
** intInArrray Function
** ----------------------------------------------------
** utility function to check if value exists in array
** params: int array[], int searchValue, int arrayLength
** returns: true if present, false if not found
** ----------------------------------------------------
*/
bool intInArray(int array[], int value, int arrayLength) {
    int i;
    /* loop through array and return true if search value is found in array */
    for (i = 0; i < arrayLength; i++) {
        if (array[i] == value) {
            return true;
        }
    }
    return false;
}

/*
** IsGraphFull Function
** ----------------------------------------------------
** utility function to check if rooms graph is full
** params: struct Room* roomsGraph[], int numberOfRooms
** returns: true if full, false if not full
** ----------------------------------------------------
*/
bool isGraphFull(struct Room roomsGraph[], int numberOfRooms)
{
    int i;
    /* loop through rooms and check if any room can have a connection added */
    for (i = 0; i < numberOfRooms; i++) {
        /* if current room has less than 3 connections, graph is not full */
        struct Room currentRoom = roomsGraph[i];
        int connections = currentRoom.numberOutboundConnections;
        if (connections < 3) {
            return false;
        }
    }
    /* all rooms have 3 connections or more so the graph is full */
    return true;
}

/*
** getRandomRoom Function
** ----------------------------------------------------
** utility function to return a random room from list of rooms
** WITHOUT validating if connection can be added
** params: struct Room rooms[], int arrayLength
** returns: struct Room
** ----------------------------------------------------
*/
struct Room* getRandomRoom(struct Room rooms[], int arrayLength)
{
    /* generate random number between 0 and 6 to pick random room */
    int randomIndex = rand() % arrayLength;
    return &rooms[randomIndex];
}

/*
** canAddConnectionFrom Function
** ----------------------------------------------------
** utility function to check if room has space for another connection
** params: struct Room room
** returns: true if # of connections is < 6
** ----------------------------------------------------
*/
bool canAddConnectionFrom(struct Room* room)
{
    /* check if room argument has less than 6 connections */
    if (room->numberOutboundConnections < 6) {
        return true;
    }
    return false;
}

/*
** connectionAlreadyExistsBetween Function
** ----------------------------------------------------
** utility function to check if connection already exists between 2 rooms
** params: (2) struct Room* room
** returns: true if connection already exists false if it doesn't
** ----------------------------------------------------
*/
bool connectionAlreadyExistsBetween(struct Room* room1, struct Room* room2)
{
    int i;
    /* loop through outbound connections to see if it already exists */
    for (i = 0; i < room1->numberOutboundConnections; i++) {
        /* if id of room 2 matches room id of room 1's outbound connection, connection already exists */
        struct Room* outboundConnection = room1->outboundConnections[i];
        if (room2->id == outboundConnection->id) {
            return true;
        }
    }
    return false;
}

/*
** connectRoom Function
** ----------------------------------------------------
** utility function to connect two rooms together (one-way only)
** params: (2) struct Room* room
** returns: void
** ----------------------------------------------------
*/
void connectRoom(struct Room* room1, struct Room* room2)
{
    /* add room2 pointer to room1's outward connections and increment counter */
    room1->outboundConnections[room1->numberOutboundConnections] = room2;
    room1->numberOutboundConnections++;
}

/*
** isSameRoom Function
** ----------------------------------------------------
** utility function to check if two rooms are the same
** params: (2) struct Room* room
** returns: true if same, false if not
** ----------------------------------------------------
*/
bool isSameRoom(struct Room* room1, struct Room* room2)
{
    /* if room id's match, rooms are the same */
    if (room1->id == room2->id) {
        return true;
    }
    return false;
}

/*
** addRandomConnection Function
** ----------------------------------------------------
** utility function to add a valid random connection
** params: list of Room structs
** returns: void
** ----------------------------------------------------
*/
void addRandomConnection(struct Room rooms[])
{
    /* create pointers to 2 rooms */
    struct Room* room1;
    struct Room* room2;

    /* keep trying a random room until finding one where we can add a connection */
    while(true) {
        room1 = getRandomRoom(rooms, 7);
        if (canAddConnectionFrom(room1) == true) {
            break;
        }
    }

    /* generate 2nd random room until finding one that:
       has room for another connection,
       is different from room 1, &
       does not already have a connection with room 1
    */
    do {
        room2 = getRandomRoom(rooms, 7);
    }
    while(canAddConnectionFrom(room2) == false ||
    isSameRoom(room1, room2) == true ||
    connectionAlreadyExistsBetween(room1, room2) == true);

    /* foumd two valid rooms for a connection, make the 2-way connection */
    connectRoom(room1, room2);
    connectRoom(room2, room1);
}

/*
** createRoomsFiles Function
** ----------------------------------------------------
** utility function to create the 7 roomes files
** params: struct Room* rooms[], int arrayLength, char* folderName]
** returns: void
** ----------------------------------------------------
*/
void createRoomsFiles(struct Room rooms[], int arrayLength, char* folderName) {
    int i;
    char fileName[64];
    /* loop through rooms */
    for (i = 0; i < arrayLength; i++) {
        /* get current room and create new data file */
        struct Room room = rooms[i];
        sprintf(fileName, "./%s/file_%d", folderName, i);
        FILE* file = fopen(fileName, "w");
        if (file != NULL) {
            /* file opened for writing, write room name */
            fprintf(file, "ROOM NAME: %s\n", room.name);

            /* loop though outbound connections and write name of each */
            int j;
            for (j = 0; j < room.numberOutboundConnections; j++) {
                fprintf(file, "CONNECTION %d: %s\n", j + 1, room.outboundConnections[j]->name);
            }

            /* write room type */
            fprintf(file, "ROOM TYPE: %s", room.roomType);
        }
        /* done writing to file */
        fclose(file);
    }
}

/*
** main Function
** ----------------------------------------------------
** Creates 7 rooms, connects them until the graph is full,
** then writes each one to a new file to be used by the game
** ----------------------------------------------------
*/
int main(int argc, char* argvp[]) {
    /* seed rand */
    srand(time(0));

    /* create folder name using template and pid */
    char folderName[32] = "mednikod.rooms";
    sprintf(folderName, "%s.%d", folderName, getpid());

    /* open directory for writing */
    int roomsCreated = mkdir(folderName, 0755);
    if (roomsCreated == 0) {
        /* int array to keep track of rooms in list */
        int rooms[7];

        /* list of actual roms */
        struct Room roomsList[7];

        /* keep making rooms untl 7 have been created */
        while (roomsCreated < 7) {
            /* get random index between 0 and 9 */
            int randomIndex = rand() % 10;

            /* if room has not already been created */
            if (!intInArray(rooms, randomIndex, roomsCreated)) {
                /* add room index to int array */
                rooms[roomsCreated] = randomIndex;

                /* create Room and set properties */
                struct Room newRoom;
                newRoom.id = roomsCreated;
                strcpy (newRoom.name, names[randomIndex]);

                /* set 1st room as START_ROOM, 2nd room as END_ROOM, and next 5 as MID_ROOM */
                if (roomsCreated == 0) {
                    strcpy(newRoom.roomType, "START_ROOM");
                } else if (roomsCreated == 1) {
                    strcpy(newRoom.roomType, "END_ROOM");
                } else {
                    strcpy(newRoom.roomType, "MID_ROOM");
                }

                /* add room to list and increment counter */
                newRoom.numberOutboundConnections = 0;
                roomsList[roomsCreated] = newRoom;
                roomsCreated++;
            }
        }
        /* add connections until no more can be added */
        while(!isGraphFull(roomsList, 7)) {
            addRandomConnection(roomsList);
        }
        /* write generated rooms list to new files */
        createRoomsFiles(roomsList, 7, folderName);
    } else {
        printf("\nThere was an error creating the directory.\n");
    }
    return 0;
}