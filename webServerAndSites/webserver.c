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

char *get_boundary(char *buffer) {
    char *ct = strstr(buffer, "Content-Type: multipart/form-data;");
    if (!ct) return NULL;

    char *boundary_str = strstr(ct, "boundary=");
    if (!boundary_str) return NULL;

    boundary_str += strlen("boundary=");

    // The boundary may be followed by \r\n or end of header
    char *end = strchr(boundary_str, '\r');
    if (!end) end = strchr(boundary_str, '\n');
    int len = end ? (end - boundary_str) : strlen(boundary_str);

    char *boundary = malloc(len + 3); // 2 for "--" + 1 for '\0'
    if (!boundary) return NULL;

    strcpy(boundary, "--");  // prefix boundary with --
    strncat(boundary, boundary_str, len);

    return boundary;
}

void handle_multipart(char *body, int bodyLength, char *boundary, char *savedFilename, size_t filenameSize) {
    // body is the POST body only, length is bodyLength
    // boundary is the multipart boundary string, e.g. "--boundary123"
    // Your parsing code here uses these parameters only

    char *part_start = body;

    while (part_start < body + bodyLength) {
        char *next_boundary = strstr(part_start, boundary);
        if (!next_boundary) break;

        // move past the boundary line
        part_start = next_boundary + strlen(boundary);

        // skip optional CRLF after boundary
        if (part_start[0] == '\r' && part_start[1] == '\n')
            part_start += 2;

        // find the next boundary (end of this part)
        next_boundary = strstr(part_start, boundary);
        if (!next_boundary) break;

        int part_len = next_boundary - part_start;

        // parse headers in this part
        char *header_end = strstr(part_start, "\r\n\r\n");
        if (!header_end) break;

        int header_len = header_end - part_start;
        char *headers = malloc(header_len + 1);
        if (!headers) break;
        memcpy(headers, part_start, header_len);
        headers[header_len] = '\0';

        char *filename_pos = strstr(headers, "filename=\"");
        if (filename_pos) {
            filename_pos += strlen("filename=\"");
            char *filename_end = strchr(filename_pos, '"');
            if (!filename_end) {
                free(headers);
                break;
            }
            int filename_len = filename_end - filename_pos;
            char filename[256];
            strncpy(filename, filename_pos, filename_len);
            filename[filename_len] = '\0';

            printf("Uploading file: %s\n", filename);

            strncpy(savedFilename, filename, filenameSize - 1);
            savedFilename[filenameSize - 1] = '\0';

            // file data starts after headers + 4 bytes (\r\n\r\n)
            char *file_data = header_end + 4;
            int file_data_len = part_len - (file_data - part_start);
            printf("Saving file: %s, size: %d bytes\n", filename, file_data_len);

            char filePath[512];
            snprintf(filePath, sizeof(filePath), "uploads/%s", filename);
            FILE *fp = fopen(filePath, "wb");
            if (fp) {
                fwrite(file_data, 1, file_data_len, fp);
                fclose(fp);
                printf("File saved successfully\n");
            } else {
                perror("fopen");
            }
        }

        free(headers);
        part_start = next_boundary + strlen(boundary);
    }
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

		if(strcmp(method, "POST") == 0 && strcmp(uri, "/api") == 0){
			char *contentLengthString = strstr(buffer, "Content-Length:");
		    int contentLength = 0;
		    if (contentLengthString){
		        sscanf(contentLengthString, "Content-Length: %d", &contentLength);
		    }

		    // Determine if this is JSON or multipart/form-data
		    char *contentType = strstr(buffer, "Content-Type:");
		    if (!contentType) {
		        close(newmysock);
		        continue;
		    }

		    // Move to value of Content-Type
		    contentType = strchr(contentType, ' ') + 1;

		    char *body = strstr(buffer, "\r\n\r\n");
		    if (!body) {
		        close(newmysock);
		        continue;
		    }
		    body += 4;

		    // Calculate how much of the body is already read
		    int bodyLength = valread - (body - buffer);

		    // Allocate full body buffer
		    char *requestBody = malloc(contentLength + 1);
		    if (!requestBody) {
		        perror("malloc");
		        close(newmysock);
		        continue;
		    }
		    requestBody[contentLength] = '\0';

		    memcpy(requestBody, body, bodyLength);

		    // Read any remaining body
		    while (bodyLength < contentLength) {
		        int bytesRead = read(newmysock, requestBody + bodyLength, contentLength - bodyLength);
		        if (bytesRead <= 0) break;
		        bodyLength += bytesRead;
		    }

		    // Handle multipart/form-data
		    if (strstr(contentType, "multipart/form-data") != NULL) {
		    	char *boundary = get_boundary(buffer);
		    	char savedFilename[256] = {0};
		    	if(boundary){
		    		handle_multipart(requestBody, contentLength, boundary, savedFilename, sizeof(savedFilename));
		    		free(boundary);
		    	}
		        
		        // Send response
		        char response[256];
				snprintf(response, sizeof(response),
				    "HTTP/1.0 200 OK\r\n"
				    "Content-Type: application/json\r\n\r\n"
				    "{\"status\":\"File uploaded\", \"filename\":\"%s\"}",
				    savedFilename
				);
		        write(newmysock, response, strlen(response));

		    } else if (strstr(contentType, "application/json") != NULL) {
		        // Save JSON as before
		        requestBody[contentLength] = '\0';

		        FILE *fp = fopen("received.json", "w");
		        if (fp) {
		        	fprintf(fp, "%s", requestBody);
		        	fclose(fp);
		        } 

		        const char *jsonResponse =
		            "HTTP/1.0 200 OK\r\n"
		            "Content-Type: application/json\r\n\r\n"
		            "{\"status\":\"JSON received\"}";
		        write(newmysock, jsonResponse, strlen(jsonResponse));
		    } else {
		        const char *badRequest =
		            "HTTP/1.0 400 Bad Request\r\n"
		            "Content-Type: text/plain\r\n\r\n"
		            "Unsupported Content-Type";
		        write(newmysock, badRequest, strlen(badRequest));
		    }

		    free(requestBody);
		    close(newmysock);
		    continue;
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