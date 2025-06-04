#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define HEADER_BUFFER_SIZE 8192

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
	}else if(strstr(filePath, ".png")){
		content_type = "image/png";
	}else if(strstr(filePath, ".jpg") || strstr(filePath, ".jpeg")){
		content_type = "image/jpeg";
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

void printHex(const char *data, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02x ", (unsigned char)data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
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
		int newmysock = accept(mysock, (struct sockaddr *)&client_addr, (socklen_t *)&client_addrlen);

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
		printf("[%s:%u]\n\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		
		//Reading the request
		//Create the method, path, and version variables
		char method[BUFFER_SIZE], uri[BUFFER_SIZE], version[BUFFER_SIZE];

		//the '%s' refers to the string formatting: each variable is formatted as a string
		sscanf(buffer, "%s %s %s", method, uri, version);

		if(strcmp(method, "POST") == 0 && strcmp(uri, "/api/chat") == 0){
			printf("%s %s %s\n", method, uri, version);
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
			printf("Content-Length: %d\n", contentLen);
			
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
			if (contentLen == bodyLen){
				printf("Body fully read\n");
			}else{
				printf("Body not fully read. Remaining bytes: %d\n", contentLen - bodyLen);
			}


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

			    // Construct boundary strings
			    char boundStart[256];
			    char boundEnd[256];
			    snprintf(boundStart, sizeof(boundStart), "--%s", boundary);
			    snprintf(boundEnd, sizeof(boundEnd), "--%s--", boundary);

			    // Boundary delimiter with preceding CRLF, used to find next part boundaries
			    char boundary_delim[260];
			    snprintf(boundary_delim, sizeof(boundary_delim), "\r\n%s", boundStart);

			    // Find the first boundary
			    char *part = strstr(body, boundStart);
			    if(!part){
			        fprintf(stderr, "No multipart boundary start found\n");
			        free(body);
			        close(newmysock);
			        continue;
			    }

			    part += strlen(boundStart) + 2; // Skip the boundary line and CRLF
			    char *filename = NULL;

			    while(part && strncmp(part, boundEnd, strlen(boundEnd)) != 0){
			        // Find the end of headers for this part
			        char *header_end = strstr(part, "\r\n\r\n");
			        if(!header_end) break;

			        int header_len = header_end - part;
			        char header_block[header_len + 1];
			        strncpy(header_block, part, header_len);
			        header_block[header_len] = '\0';
			        printf("\nheaderBlock: \n%s\n", header_block);


			        //Look at the name part of the multipart/form-data request
			        //Extracting name and filename
			        char name[128] = {0};
			        char *contDis = strstr(header_block, "Content-Disposition");
					
					if (contDis) {
					    char *name_start = strstr(contDis, "name=\"");
					    if (name_start) {
					        name_start += 6; // Move past 'name="'
					        char *name_end = strchr(name_start, '"');
					        if (name_end) {
					            size_t name_len = name_end - name_start;
					            if (name_len < sizeof(name)) {
					                strncpy(name, name_start, name_len);
					                name[name_len] = '\0';
					            }
					        }
					    }
					}
					printf("name: %s\n", name);

			        // Extract filename if present
			        char *thisFilename = NULL;
		        	char *cd = strstr(header_block, "Content-Disposition:");
			        if(cd){
			            char *fn_start = strstr(cd, "filename=\"");
			            if(fn_start){
			                fn_start += 10;
			                char *fn_end = strchr(fn_start, '"');
			                if(fn_end){
			                    size_t fn_len = fn_end - fn_start;
			                    thisFilename = malloc(fn_len + 1);
			                    strncpy(thisFilename, fn_start, fn_len);
			                    thisFilename[fn_len] = '\0';
			                }
			            }
			        }
			        
			        printf("\nfilename: %s\n", thisFilename ? thisFilename : "(null)");

			        // Data starts after headers + 4 for \r\n\r\n
			        char *data_start = header_end + 4;

			        // Find next part boundary (with CRLF before boundary)
			        char *next_part = strstr(data_start, boundary_delim);
			        int data_len;
			        if(next_part){
			            data_len = next_part - data_start;
			        }else{
			            // Last part: data till end of body
			            data_len = contentLen - (data_start - body);
			        }

			        // Trim trailing CRLF from data
			        while(data_len > 0 && (data_start[data_len - 1] == '\r' || data_start[data_len - 1] == '\n')){
			            data_len--;
			        }
			        printf("\nData length after trimming: %d\n", data_len);

			        if(thisFilename){
			            printf("Attempting to save!\n");
			            char filepath[256];
			            snprintf(filepath, sizeof(filepath), "uploads/%s", thisFilename);
			            printf("filepath: %s\n", filepath);

			            FILE *fp = fopen(filepath, "wb");
			            if(fp){
			                fwrite(data_start, 1, data_len, fp);
			                fclose(fp);
			                printf("Saved file: %s (%d bytes)\n", filepath, data_len);
			            }else{
			                perror("Failed to save file");
			            }

			            free(filename);
			            filename = strdup(thisFilename);
			            free(thisFilename);
			        }

			        // Move to the next part: skip \r\n before boundary if present
					if(next_part){
					    part = next_part + strlen(boundary_delim);
					}else{
					    break;
					}
			    }

			    free(body);

			    if(filename){
			    	char jsonResp[512];
					snprintf(jsonResp, sizeof(jsonResp),
					    "HTTP/1.0 200 OK\r\n"
					    "Content-Type: application/json\r\n\r\n"
					    "{\"status\": \"success\", \"filename\": \"%s\"}\n", filename);

					printf("jsonResp:\n%s\n", jsonResp);
					write(newmysock, jsonResp, strlen(jsonResp));
					free(filename);
			    }else{
			    	char *failResp = "HTTP/1.0 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\nFile upload failed\n";
    				write(newmysock, failResp, strlen(failResp));
			    }
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
		    "Access-Control-Allow-Origin: *\r\n"
		    "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
		    "Access-Control-Allow-Headers: Content-Type\r\n"
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
