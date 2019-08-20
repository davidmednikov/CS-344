#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    char threeStr[3] = "ab";
    strcpy(threeStr, "abc");
    printf(threeStr);
    return 0;
}