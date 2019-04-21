#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// Room struct
// ----------------------------------------------------
// struct to hold all attributes of adventure room
// ----------------------------------------------------
struct Room {
    int id;
    char* name;
    char* roomType;
    int numOutboundConnections;
    struct room* outboundConnections[6];
};

/* intInArrray Function
** ----------------------------------------------------
** utility function to check if value exists in array
** params: int array[], int searchValue, int arrayLength
** returns: 1 if present, 0 if not found
** ----------------------------------------------------
*/
int intInArray(int array[], int value, int arrayLength) {
    int i;
    for (i = 0; i < arrayLength; i++) {
        if (array[i] == value) {
            return 1;
        }
    }
    return 0;
}

char names[10][9] = { "Assembly", "Crystal", "Elixir", "Mercury", "Perl", "Python", "React", "Ring", "Ruby", "SPARK" };

int main(int argc, char* argvp[]) {
    srand(time(0));

    char* folderName = calloc(22, sizeof(char));
    sprintf(folderName, "mednikod.rooms.%d", getpid());
    int roomsCreated = mkdir(folderName);
    if (roomsCreated == 0) {
        int rooms[7];
        while (roomsCreated < 7) {
            int randomIndex = rand() % 10;
            if (!intInArray(rooms, randomIndex, roomsCreated)) {
                rooms[roomsCreated] = randomIndex;
                struct Room newRoom;
                newRoom.id = roomsCreated;
                newRoom.name = calloc(sizeof(names[randomIndex]), sizeof(char));
                strcpy (newRoom.name, names[randomIndex]);
                if (roomsCreated == 0) {
                    newRoom.roomType = calloc(sizeof("START_ROOM"), sizeof(char));
                    strcpy(newRoom.roomType, "START_ROOM");
                } else if (roomsCreated == 1) {
                    newRoom.roomType = calloc(sizeof("END_ROOM"), sizeof(char));
                    strcpy(newRoom.roomType, "END_ROOM");
                } else {
                    newRoom.roomType = calloc(sizeof("MID_ROOM"), sizeof(char));
                    strcpy(newRoom.roomType, "MID_ROOM");
                }
                printf("The new room id%d is %s and is type %s\n", newRoom.id, newRoom.name, newRoom.roomType);
                roomsCreated++;
            }
        }
    } else {
        printf("\nThere was an error creating the directory.\n");
    }
}