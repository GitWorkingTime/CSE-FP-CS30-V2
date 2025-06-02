#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/types.h>

#define PORT 8080
#define BUFFER_SIZE 1024

/*
This local web server can be accessed via:
	http://localhost:8080
*/

int createSocket(){
	int mysock = socket(AF_INET, SOCK_STREAM, 0);
	if (mysock == -1){
		perror("webserver (socket)");
		return 1;
	}
	printf("socket created successfully\n");
	printf("\n");
	return mysock;
}

int saveJSONToFile(const char *filepath, const char *JSONData) {
    FILE *fp = fopen(filepath, "w");  // Open file for writing (overwrite)
    if (fp == NULL) {
        perror("Failed to open file for writing");
        return -1;
    }

    if (fprintf(fp, "%s", JSONData) < 0) {
        perror("Failed to write JSON to file");
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

void printAddressProperties(struct sockaddr_in host_addr){
	printf("host_addr.sin_family: \t %d\n", host_addr.sin_family);
	printf("host_addr.sin_port: \t %d\n", host_addr.sin_port);
	printf("host_addr.sin_addr.s_addr: \t %d\n", host_addr.sin_addr.s_addr);
	printf("host IP address and Port \t [%s:%u]\n", inet_ntoa(host_addr.sin_addr), ntohs(host_addr.sin_port));
	printf("\n");
}

char *getContentType(char *filePath){
	//Determining content-type (a.k.a MIME TYPE)
	char *content_type = "text/plain"; //Default type

	//strstr is used to find the first instance of a selected string
	if (strstr(filePath, ".html"))
		content_type = "text/html";
	else if (strstr(filePath, ".css"))
		content_type = "text/css";
	else if(strstr(filePath, ".js")){
		content_type = "application/javascript";
	}
	return content_type;
}

void ensureUploadsFolderExists() {
    struct stat st = {0};

    if (stat("uploads", &st) == -1) {
        if (mkdir("uploads", 0755) == 0) {
            printf("Created 'uploads' directory.\n");
        } else {
            perror("Failed to create 'uploads' directory");
        }
    } else {
        printf("'uploads' directory already exists.\n");
    }
}

int initServer(char *response){
	ensureUploadsFolderExists();

	char buffer[BUFFER_SIZE];
	char *resp = response;
	// printf("%s", resp);

	//Create socket
	int mysock = createSocket();

	//Create the address for the socket to bind onto
	struct sockaddr_in host_addr;
	int host_addrlen = sizeof(host_addr);

	//Setting the address properties
	host_addr.sin_family = AF_INET;
	host_addr.sin_port = htons(PORT);
	host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//Create the client address
	struct sockaddr_in client_addr;
	int client_addrlen = sizeof(client_addr);

	//Binding the socket to the address
	// The if function serves to check for an error (i.e if the bind function returns a -1)
	if (bind(mysock, (struct sockaddr *)&host_addr, host_addrlen) != 0){
		perror("webserver (bind)");
		return 1;
	}
	printf("socket successfully bound to address\n");
	printf("\n");

	//Listening for incoming connections
	//The if function serves to check for an error (i.e if the listen function returns a -1)
	if (listen(mysock, 128) != 0){
		perror("webserver (listen)");
		return 1;
	}
	printf("server listening for connections right now\n");
	printf("\n");

	//Acts the same as a while loop. 
	//Avoids compiler issues compared to while(true) due to the constant boolean expression (i.e "true")
	for(;;){
		//Accept incoming connections
		int newmysock = accept(mysock, (struct sockaddr *)&host_addr, (socklen_t *)&host_addrlen);

		//If the newmysock returns an error(a.k.a -1)
		if(newmysock < 0){
			perror("webserver (accept)");

			//Go to the next socket queued
			continue; 
		}
		printf("connnection accepted:\n");

		//Get client address
		int clientsock = getsockname(newmysock, (struct sockaddr *)&client_addr, (socklen_t *)&client_addrlen);

		if (clientsock < 0){
			perror("webserver (getsockname");
			continue;
		}

		//Read the accepted socket:
		int valread = read(newmysock, buffer, BUFFER_SIZE);

		//Again, if the read returns an error (i.e a -1), skip to the next accepted socket
		if (valread < 0){
			perror("webserver (read");
			continue;
		}

		//Printing the IP address and port of the client after it has been read
		printf("[%s:%u]\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		
		//Reading the request
		//Create the method, path, and version variables
		char method[BUFFER_SIZE], uri[BUFFER_SIZE], version[BUFFER_SIZE];

		//the '%s' refers to the string formatting: each variable is formatted as a string
		sscanf(buffer, "%s %s %s", method, uri, version);

		if(strcmp(method, "POST") == 0 && strcmp(uri, "/api/chat") == 0){
			//Find content-length from headers
			int contentLen = 0;
			char *contLenPtr = strstr(buffer, "Content-Length:");

			//Find the content-type from the header
			char *contentTypePtr = strstr(buffer, "Content-Type:");
			char contentType[128] = {0};
			char boundary[128] = {0};

			if(contentTypePtr){
				//Reads the formatted input (i.e contentType) and stores it in contentTypePtr
				sscanf(contentTypePtr, "Content-Type: %127[^\r\n]", contentType);

				if(strstr(contentType, "multipart/form-data") != NULL){
					char *bound = strstr(contentType, "boundary=");
					if(bound){
						strcpy(boundary, bound + strlen("boundary="));

						//Boundary may be quoted, strip quotes if present:
						if(boundary[0] == '"'){
							memmove(boundary, boundary+1, strlen(boundary));
							boundary[strlen(boundary) - 1] = '\0';
						}
					}else{
						fprintf(stderr, "No boundary found in multipart/form-data\n");
						close(newmysock);
						continue;
					}
				}

			}

			if(contLenPtr){
				sscanf(contLenPtr, "Content-Length: %d", &contentLen);
			}
			
			char *bodyStart = strstr(buffer, "\r\n\r\n");
			if(!bodyStart){
				fprintf(stderr, "Invalid POST request: missing body\n");
				close(newmysock);
				continue;
			}
			bodyStart += 4;

			int headerLen = bodyStart - buffer;
			int bodyLen = valread - headerLen;

			char *body = malloc(contentLen + 1);
			memcpy(body, bodyStart, bodyLen);

			//If body not complete, read the rest
			while (bodyLen < contentLen){
				int bytes = read(newmysock, body + bodyLen, contentLen - bodyLen);
				if(bytes <= 0){
					break;
				}
				bodyLen += bytes;
			}
			body[bodyLen] = '\0';

			if(strstr(contentType, "application/json") != NULL){
				printf("Received JSON: \n%s\n", body);
				if(saveJSONToFile("uploads/received.json", body) == 0){
					printf("JSON saved successfully!\n");
				}else{
					fprintf(stderr, "Failed to save JSON file. \n");
				}

				char *okResp = "HTTP/1.0 200 OK\r\n"
							"Content-Type: application/json\r\n\r\n"
							"{\"status\": \"success\"}";

				write(newmysock, okResp, strlen(okResp));
				free(body);
				close(newmysock);
				continue;
			}else if(strstr(contentType, "multipart/form-data") != NULL){
				printf("Content is multipart/form-data\n");

				//Construct boundary strings
				char boundStart[256];
				char boundEnd[256];
				snprintf(boundStart, sizeof(boundStart), "--%s", boundary);
				snprintf(boundEnd, sizeof(boundEnd), "--%s--", boundary);

				char *partStart = strstr(body, boundStart);
				if(!partStart){
					fprintf(stderr, "No multipart boundary start found\n");
					free(body);
					close(newmysock);
					continue;
				}

				printf("boundary start: %s\n boundary end: %s\n", boundStart, boundEnd);

				partStart += strlen(boundStart) + 2; //Skip boundary and CLRF

				//Find the next boundary (end of this part)
				char *partEnd = strstr(partStart, boundStart);
				if(!partEnd){
					partEnd = strstr(partStart, boundEnd);
					if(!partEnd){
						fprintf(stderr, "No mulipart boundary end found\n");
						free(body);
						close(newmysock);
						continue;
					}
				}

				//Extract headers of the part
				char *headEnd = strstr(partStart, "\r\n\r\n");
				if(!headEnd){
					fprintf(stderr, "Invalid multipart format: no header/body seperation\n");
					free(body);
					close(newmysock);
					continue;
				}

				int headLen = headEnd - partStart;
				int dataLen = partEnd - (headEnd + 4);

				char headers[headLen + 1];
				strncpy(headers, partStart, headLen);

				//Parsing content-disposition to get filename
				char *filenamePtr= strstr(headers, "filename=\"");
				char filename[256] = {0};
				if(filenamePtr){
					filenamePtr + strlen("filename=\"");
					char *filenameEnd = strchr(filenamePtr, '"');
					if (filenameEnd){
						int filenameLen = filenameEnd - filenamePtr;
						strncpy(filename, filenamePtr, filenameLen);
						filename[filenameLen] = '\0';
					}
				}else{
					fprintf(stderr, "No filename in multipart part\n");
					free(body);
					close(newmysock);
					continue;
				}

				//Saving the file content as raw bytes
				char filepath[512];
				snprintf(filepath, sizeof(filepath), "uploads/%s", filename);

				printf("filepath: %s\n", filepath);
				FILE *fp = fopen(filepath, "wb");
				if(!fp){
					perror("failed to open file for writing uploaded data (fopen)");
					printf("Failed to open file\n");
					free(body);
					close(newmysock);
					continue;
				}
				printf("File created\n");

				fwrite(headEnd + 4, 1, dataLen, fp);
				fclose(fp);

				printf("Saved uploaded file: %s\n", filepath);


				char *okResp = "HTTP/1.0 200 OK\r\n"
							"Content-Type: application/json\r\n\r\n"
							"{\"status\": \"success\", \"file\": \"uploaded\"}";

				write(newmysock, okResp, strlen(okResp));
				free(body);
				close(newmysock);
				continue;
			}
		}

		//Printing the request header
		printf("%s %s %s\n", 
			   method,
			   version,
			   uri);

		printf("\n");


		char filePath[BUFFER_SIZE] = "."; //This represents the current directory
		strcat(filePath, uri); //Append onto the filePath the file chosen

		if (strcmp(uri, "/") == 0){ //strcmp compares two strings to see if a certain character(s) exists in both
			strcpy(filePath, "./index.html");
		}

		FILE *requestedFile = fopen(filePath, "r");

		//IF the file is not found, send a 404 error
		if (requestedFile == NULL) {
			char *not_found = "HTTP/1.0 404 Not Found\r\n"
			                  "Content-Type: text/html\r\n\r\n"
			                  "<h1>404 Not Found</h1>";
			write(newmysock, not_found, strlen(not_found));
			close(newmysock);
			continue; //Go to the next iteration
		}

		char *content_type = getContentType(filePath);

		//Get file size
		fseek(requestedFile, 0, SEEK_END);
		long fsize = ftell(requestedFile);
		rewind(requestedFile);

		//Reading file content
		char *fileContent = malloc(fsize + 1);
		fread(fileContent, 1, fsize, requestedFile);
		fileContent[fsize] = 0;
		fclose(requestedFile);

		char header[BUFFER_SIZE];
		snprintf(header, sizeof(header),
			"HTTP/1.0 200 OK\r\n"
			"Content-Type: %s\r\n"
			"Content-Length: %ld\r\n\r\n",
			content_type, fsize);

		//Writing to the server:
		int writeHeader = write(newmysock, header, strlen(header));
		if(writeHeader < 0){
			perror("webserver (write)");
			continue;
		}

		int writeFile = write(newmysock, fileContent, fsize);
		if(writeFile < 0){
			perror("webserver (write)");
			continue;
		}

		free(fileContent);
		close(newmysock);
	}
	return 0;
}