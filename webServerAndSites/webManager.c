// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include "webserver.c"

// //source: https://stackoverflow.com/questions/6280055/how-do-i-check-if-a-variable-is-of-a-certain-type-compare-two-types-in-c
// #define typename(x) _Generic((x),                                                 \
//             _Bool: "_Bool",                  unsigned char: "unsigned char",          \
//              char: "char",                     signed char: "signed char",            \
//         short int: "short int",         unsigned short int: "unsigned short int",     \
//               int: "int",                     unsigned int: "unsigned int",           \
//          long int: "long int",           unsigned long int: "unsigned long int",      \
//     long long int: "long long int", unsigned long long int: "unsigned long long int", \
//             float: "float",                         double: "double",                 \
//       long double: "long double",                   char *: "pointer to char",        \
//            void *: "pointer to void",                int *: "pointer to int",         \
//           default: "other")

// // https://www.geeksforgeeks.org/how-to-append-a-character-to-a-string-in-c/ source for appending strings


// int main(){
//     // char *fileContent;
//     // unsigned long size = getFileSize("index.html");
//     // fileContent = extractFileContentTxt("index.html");
//     extractFileContentTxt("index.html");

//     // char resp[999999] = "HTTP/1.0 200 OK\r\n"
//     //                   "Server: webserver-c\r\n"
//     //                   "Content-type: text/html\r\n\r\n";

//     // printf("%s \n", fileContent);

//     // strcat(resp, fileContent);
//     // strcat(resp, "\r\n");
//     // // printf("%s \n", resp);
//     // free(fileContent);
//     // // initServer(resp);

//     return 0;
// }

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "webserver.c"

#define BUFFER_SIZE 1024

unsigned long getFileSize(const char *filePath) {
    FILE *file = fopen(filePath, "r");
    if (file == NULL) {
        perror("cannot open file");
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    fclose(file);
    return (unsigned long)size;
}

char *extractFileContentTxt(const char *filePath) {
    unsigned long size = getFileSize(filePath);
    if (size == 0) {
        return NULL;
    }

    FILE *file = fopen(filePath, "r");
    if (file == NULL) {
        perror("cannot open file");
        return NULL;
    }

    // Dynamically allocate memory to hold the content of the file
    char *content = (char *)malloc(size + 1); // +1 for the null terminator
    if (content == NULL) {
        perror("memory allocation failed");
        fclose(file);
        return NULL;
    }

    size_t readSize = fread(content, 1, size, file);
    if (readSize != size) {
        perror("reading file failed");
        free(content);
        fclose(file);
        return NULL;
    }

    content[size] = '\0';  // Null-terminate the string

    fclose(file);
    return content;
}

int main() {
    char *fileContent = extractFileContentTxt("index.html");
    char resp[getFileSize("index.html") + 73];
    strcat(resp, "HTTP/1.0 200 OK\r\n");
    strcat(resp,"Server: webserver-c\r\n");
    strcat(resp, "Content-type: text/html\r\n\r\n");

    if (fileContent) {
        // printf("File content:\n%s\n", fileContent);
        
        strcat(resp, fileContent);
        strcat(resp, "\r\n");

        printf("resp: %s\n", resp);





        free(fileContent);  // Don't forget to free the allocated memory!
        initServer(resp);

    } else {
        printf("Failed to read file content.\n");
    }

    return 0;
}
