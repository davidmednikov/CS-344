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
    for (i = 0; i < numberOfRooms; i++) {
        struct Room currentRoom = roomsGraph[i];
        int connections = currentRoom.numberOutboundConnections;
        if (connections < 3) {
            return false;
        }
    }
    return true;
}

/*
** IsGraphFull Function
** ----------------------------------------------------
** utility function to check if rooms graph is full
** params: struct Room* roomsGraph[], int numberOfRooms
** returns: true if full, false if not full
** ----------------------------------------------------
*/
void printRooms(struct Room rooms[], int numberOfRooms) {
    int i;
    for (i = 0; i < numberOfRooms; i++) {
        struct Room room = rooms[i];
        printf("%d The room id%d is %s and is type %s\n", i, room.id, room.name, room.roomType);
    }
}

void printConnections(struct Room rooms[], int numberOfRooms) {
    int i;
    for (i = 0; i < numberOfRooms; i++) {
        struct Room room = rooms[i];
        printf("%d The room id%d is %s and is type %s\n", i, room.id, room.name, room.roomType);
        int j;
        for (j = 0; j < room.numberOutboundConnections; j++) {
            struct Room* connection = room.outboundConnections[j];
            printf("%d %s room id%d connects to %s room id%d with type %s\n", i, room.name, room.id, connection->name, connection->id, connection->roomType);
        }
    }
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
    for (i = 0; i < room1->numberOutboundConnections; i++) {
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
  struct Room* room1;
  struct Room* room2;

  while(true)
  {
    room1 = getRandomRoom(rooms, 7);

    if (canAddConnectionFrom(room1) == true)
      break;
  }

  do
  {
    room2 = getRandomRoom(rooms, 7);
  }
  while(canAddConnectionFrom(room2) == false || isSameRoom(room1, room2) == true || connectionAlreadyExistsBetween(room1, room2) == true);

  connectRoom(room1, room2);
  connectRoom(room2, room1);
}

void createRoomsFiles(struct Room rooms[], int arrayLength, char* folderName) {
    int i;
    char fileName[64] = "";
    for (i = 0; i < arrayLength; i++) {
        struct Room room = rooms[i];
        sprintf(fileName, "./%s/file_%d", folderName, i);
        FILE* file = fopen(fileName, "w");
        if (file != NULL) {
            fprintf(file, "ROOM NAME: %s\n", room.name);
            int j;
            for (j = 0; j < room.numberOutboundConnections; j++) {
                fprintf(file, "CONNECTION %d: %s\n", j + 1, room.outboundConnections[j]->name);
            }
            fprintf(file, "ROOM TYPE: %s", room.roomType);
        }
        fclose(file);
    }
}

char names[10][9] = { "Assembly", "Crystal", "Elixir", "Mercury", "Perl", "Python", "React", "Ring", "Ruby", "SPARK" };

int main(int argc, char* argvp[]) {
    srand(time(0));
    char folderName[32] = "mednikod.rooms";
    sprintf(folderName, "%s.%d", folderName, getpid());
    int roomsCreated = mkdir(folderName, 0755);
    if (roomsCreated == 0) {
        int rooms[7];
        struct Room roomsList[7];
        while (roomsCreated < 7) {
            int randomIndex = rand() % 10;
            if (!intInArray(rooms, randomIndex, roomsCreated)) {
                rooms[roomsCreated] = randomIndex;
                struct Room newRoom;
                newRoom.id = roomsCreated;
                strcpy (newRoom.name, names[randomIndex]);
                if (roomsCreated == 0) {
                    strcpy(newRoom.roomType, "START_ROOM");
                } else if (roomsCreated == 1) {
                    strcpy(newRoom.roomType, "END_ROOM");
                } else {
                    strcpy(newRoom.roomType, "MID_ROOM");
                }
                newRoom.numberOutboundConnections = 0;
                roomsList[roomsCreated] = newRoom;
                roomsCreated++;
            }
        }
        while(!isGraphFull(roomsList, 7)) {
            addRandomConnection(roomsList);
        }
        createRoomsFiles(roomsList, 7, folderName);
    } else {
        printf("\nThere was an error creating the directory.\n");
    }
    return 0;
}