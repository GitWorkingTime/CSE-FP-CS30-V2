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
#define BUFFER_SIZE 4096

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
	if (strstr(filePath, ".html")){
		content_type = "text/html";
	}
	else if (strstr(filePath, ".css")){
		content_type = "text/css";
	}
	else if(strstr(filePath, ".js")){
		content_type = "application/javascript";
	}
	else if (strstr(filePath, ".jpg") || strstr(filePath, ".jpeg")){
		content_type = "image/jpeg";
	}
	else if (strstr(filePath, ".png")){
		content_type = "image/png";
	}
	else if (strstr(filePath, ".gif")){
		content_type = "image/gif";
	}

	return content_type;
}

int initServer(char *response){
	mkdir("uploads", 0777);
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
		printf("connnection accepted: %d\n", newmysock);

		//Get client address
		int clientsock = getsockname(newmysock, (struct sockaddr *)&client_addr, (socklen_t *)&client_addrlen);

		if (clientsock < 0){
			perror("webserver (getsockname");
			continue;
		}

		//Read the accepted socket:
		int totalRead = 0;
		while(1){
			int bytesRead = read(newmysock, buffer + totalRead, sizeof(buffer) - totalRead - 1);
			if (bytesRead <= 0){
				break;
			}

			totalRead += bytesRead;
			if(totalRead >= sizeof(buffer - 1)){
				break;
			}
		}
		buffer[totalRead] = '\0'; 

		// //Again, if the read returns an error (i.e a -1), skip to the next accepted socket
		// if (valread < 0){
		// 	perror("webserver (read");
		// 	continue;
		// }

		//Printing the IP address and port of the client after it has been read
		printf("[%s:%u]\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		
		//Reading the request
		//Create the method, path, and version variables
		char method[BUFFER_SIZE], uri[BUFFER_SIZE], version[BUFFER_SIZE];

		//the '%s' refers to the string formatting: each variable is formatted as a string
		sscanf(buffer, "%s %s %s", method, uri, version);

		if(strmp(method, "GET") == 0 && strcmp(uri, "/chat") == 0){
			FILE *jsonFile = fopen("uploads/data.json", "r");
			if(!jsonFile){
		        char *not_found = "HTTP/1.0 404 Not Found\r\n"
                  "Content-Type: application/json\r\n\r\n"
                  "{\"error\": \"Data not found\"}";
		        write(newmysock, not_found, strlen(not_found));
		        close(newmysock);
		        continue;
			}

			//Reading the JSON contents
			fseek(jsonFile, 0, SEEK_END);
			long fileSize = ftell(jsonFile);
			rewind(jsonFile);

			char *jsonContent = malloc(fileSize + 1);
			fread(jsonContent, 1, fileSize, jsonFile);
			jsonContent[fileSize] = 0;
			fclose(jsonFile);
			
		    char header[BUFFER_SIZE];
		    snprintf(header, sizeof(header),
		        "HTTP/1.0 200 OK\r\n"
		        "Content-Type: application/json\r\n"
		        "Content-Length: %ld\r\n\r\n", fsize);

		    write(newmysock, header, strlen(header));
		    write(newmysock, jsonContent, fsize);
		    free(jsonContent);
		    close(newmysock);
		    continue;
		}

		if(strcmp(method, "POST") == 0 && strstr(uri, "/") != NULL){
			char *contentTypeHeader = strstr(buffer, "Content-Type: multipart/form-data;");
			if(contentTypeHeader){
				char *boundaryStart = strstr(contentTypeHeader, "boundary=");
				if(!boundaryStart){
					continue;
				}

				char  boundary[256];
				sscanf(boundaryStart, "boundary=%s", boundary);

				//Form boundary string with '--'
				char fullBoundary[260];
				snprintf(fullBoundary, sizeof(fullBoundary), "--%s", boundary);

				//Find start of body
				char *body = strstr(buffer, "\r\n\r\n");
				if(!body){
					continue;
				}
				body += 4;

				//Parsing for the message part
				char *messagePart = strstr(body, "name=\"message\"");
				char message[1024] = {0};
				if(messagePart){
					char *msgStart = strstr(messagePart, "\r\n\r\n");
					if(msgStart){
						msgStart += 4;
						char *msgEnd = strstr(msgStart, fullBoundary);
						if(msgEnd && (msgEnd - msgStart) < sizeof(message)){
							strncpy(message, msgStart, msgEnd - msgStart - 2);
							message[msgEnd - msgStart - 2] = '\0';
						}
					}
				}

				//Parsing for the image part
				char *imagePart = strstr(body, "name=\"image\"");
				if(imagePart){
					//Extract filename
					char *filenameStart = strstr(imagePart, "filename=\"");
					if(!filenameStart){
						continue;
					}
					filenameStart += 10;

					char *filenameEnd = strchr(filenameStart, '"');
					char filename[256] = {0};
					strncpy(filename, filenameStart, filenameEnd - filenameStart);

					//Find the image data
					char *imgDataStart = strstr(imagePart, "\r\n\r\n");
					if(!imgDataStart){
						continue;
					}
					imgDataStart += 4;

					//Find the end of the image data using the boundary
					char *imgDataEnd = strstr(imgDataStart, fullBoundary);
					if(!imgDataEnd){
						continue;
					}
					long imgSize = imgDataEnd - imgDataStart - 2;

					//Save the image to file
					char imgPath[512];
					snprintf(imgPath, sizeof(imgPath), "uploads/%s", filename);
					FILE *imgFile = fopen(imgPath, "wb");
					if (imgFile == NULL){
						printf("image file not opened!");
					}

					if(imgFile){
						fwrite(imgDataStart, 1, imgSize, imgFile);
						fclose(imgFile);
					}

					//Save the message to JSON
					FILE *jsonFile = fopen("uploads/data.json", "w");
					if(jsonFile){
						fprintf(jsonFile, 
							"{\n  \"message\": \"%s\",\n  \"image\": \"%s\"\n}\n"
							, message, filename);
						fclose(jsonFile);
					}
					// Respond
		            char *response = "HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\n\r\nMessage and image saved.";
		            write(newmysock, response, strlen(response));

				}

			}

		}

		//Printing the request header
		printf("%s %s %s\n", 
			   method,
			   version,
			   uri);

		printf("\n");


		char filePath[BUFFER_SIZE] = "."; //This represents the current directory
		if(strncmp(uri, "/uploads/", 9) == 0){
			snprintf(filePath, sizeof(filePath), ".%s", uri);
		}else if(strcmp(uri, "/") == 0){
			strcpy(filePath, "./index.html");
		}else{
			snprintf(filePath, sizeof(filePath), ".%s", uri);
		}



		if (strcmp(uri, "/") == 0){ //strcmp compares two strings to see if a certain character(s) exists in both
			strcpy(filePath, "./index.html");
		}

		const char *mode = strstr(filePath, ".jpg") || strstr(filePath, ".png") ? "rb" : "r";
		FILE *requestedFile = fopen(filePath, mode);

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