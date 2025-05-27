#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "webserver.c"


// https://www.geeksforgeeks.org/how-to-append-a-character-to-a-string-in-c/ source for appending strings

int main(){
    // initServer();

    FILE* index;
    index = fopen("index.html", "r");
    if (index == NULL){
        printf("cannot get file \n");
    }

    // printf("%ld \n", file_size);

    long size = 0;
    char ch;
    do{
        ch = fgetc(index);
        size += 1;

    }while(ch != EOF);
    printf("%ld \n", size);


    char line[size];
    char file[size];
    while(fgets(line, size, index)){
        strcat(file, line);
        // printf("%s", line);
    };

    printf("%s", file);

    fclose(index);



    return 0;
}