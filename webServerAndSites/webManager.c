#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "webserver.c"


// https://www.geeksforgeeks.org/how-to-append-a-character-to-a-string-in-c/ source for appending strings

unsigned long getFileSize(char *fileP);

char *extractFileContentTxt(char *filePath){
    // printf("%s \n", filePath);

    unsigned long size = getFileSize(filePath);
    // printf("size: %ld \n", size);

    FILE *file;
    file = fopen(filePath, "r");
    if(file == NULL){
        printf("cannot get file\n");
        return "NULL";
    }

    char txtFile[size];
    char line[size];
    while(fgets(line, size, file)){
        strcat(txtFile, line);
    }
    // printf("%s\n", txtFile);

    fclose(file);

    char *txt = (char*)malloc(size + 1);
    if (txt == NULL){
        printf("not allocated properly \n");
        return "NULL";
    }
    strcpy(txt, txtFile);
    return txt;
}

unsigned long getFileSize(char *filePath){
    FILE *file;
    file = fopen(filePath, "r");
    if(file == NULL){
        printf("cannot get file\n");
        return 0;
    }

    //Calculating size:
    long size = 0;
    char c;
    do{

        c = fgetc(file);
        size++;

    }while(c != EOF);

    //Close it after calculating size.
    fclose(file);
    return size;
}

int main(){
    // initServer();
    printf("file content:\n %s \n", extractFileContentTxt("index.html"));

    return 0;
}