#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "webserver.c"

//source: https://stackoverflow.com/questions/6280055/how-do-i-check-if-a-variable-is-of-a-certain-type-compare-two-types-in-c
#define typename(x) _Generic((x),                                                 \
            _Bool: "_Bool",                  unsigned char: "unsigned char",          \
             char: "char",                     signed char: "signed char",            \
        short int: "short int",         unsigned short int: "unsigned short int",     \
              int: "int",                     unsigned int: "unsigned int",           \
         long int: "long int",           unsigned long int: "unsigned long int",      \
    long long int: "long long int", unsigned long long int: "unsigned long long int", \
            float: "float",                         double: "double",                 \
      long double: "long double",                   char *: "pointer to char",        \
           void *: "pointer to void",                int *: "pointer to int",         \
          default: "other")

// https://www.geeksforgeeks.org/how-to-append-a-character-to-a-string-in-c/ source for appending strings

#define BUFFER_SIZE 1024

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

    char txtFile[size + 1];
    char line[size + 1];
    while(fgets(line, size, file)){
        strcat(txtFile, line);
    }
    strcat(txtFile, "\0");
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
    file = fopen(filePath, "rb");
    if(file == NULL){
        printf("cannot get file\n");
        return 0;
    }

    //Calculating size:
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    //Close it after calculating size.
    fclose(file);
    return fileSize + 1;
}

int test(char *string){
    // printf("%s \n", string);
    char *i = string;
    printf("testing\n%s\n", i);
    return 0;
}   

int main(){
    char *fileContent;
    unsigned long size = getFileSize("index.html");
    fileContent = extractFileContentTxt("index.html");

    char resp[999999] = "HTTP/1.0 200 OK\r\n"
                      "Server: webserver-c\r\n"
                      "Content-type: text/html\r\n\r\n";

    printf("%s \n", fileContent);

    strcat(resp, fileContent);
    strcat(resp, "\r\n");
    // printf("%s \n", resp);
    free(fileContent);
    // initServer(resp);

    return 0;
}