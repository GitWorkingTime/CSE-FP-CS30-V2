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
#define BUFFER_SIZE 2048
#define HEADER_BUFFER_SIZE 8192

//List of connected SSE clients
#define MAX_CLIENTS 100
int sse_clients[MAX_CLIENTS];
int sse_client_count = 0;

void sendSSEUpdate(const char *jsonData) {
    char msg[BUFFER_SIZE];
    snprintf(msg, sizeof(msg), "data: %s\n\n", jsonData);  // SSE format: 'data: <jsonData>\n\n'

    for (int i = 0; i < sse_client_count; i++) {
        int client_fd = sse_clients[i];
        // Try sending to the client
        if (write(client_fd, msg, strlen(msg)) < 0) {
            // If the write fails, remove the client
            close(client_fd);
            sse_clients[i] = sse_clients[--sse_client_count];
            i--;  // Adjust index after removal
        }
    }
}

// Function to handle SSE connections
void handleSSEConnection(int newmysock) {
    // Send the necessary SSE headers
    char *header = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/event-stream\r\n"
                   "Cache-Control: no-cache\r\n"
                   "Connection: keep-alive\r\n\r\n";
    write(newmysock, header, strlen(header));

    // Add this socket to the list of SSE clients
    if (sse_client_count < MAX_CLIENTS) {
        sse_clients[sse_client_count++] = newmysock;
    } else {
        printf("Max SSE clients reached. Ignoring new client.\n");
    }
}
/*
This local web server can be accessed via:
	http://localhost:8080

Issues so far:
- Able to POST a text as JSON and file as multipart/form-data seperately but not together at the same time

TO-DO:
- transfer data to different clients/update other clients whenever files changes
- better HTML webpage + CSS

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
	}else if(strstr(filePath, ".json")){
		content_type = "application/json";
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

//From GeeksforGeeks:
char* deleteCharC(char* s, char ch) {
    int i, j;
    int len = strlen(s);
    for (i = j = 0; i < len; i++) {
        if (s[i] != ch) {
            s[j++] = s[i];
        }
    }
    s[j] = '\0';
    return s;
}

char *extract_filename(const char *header_block) {
    const char *cd = strstr(header_block, "Content-Disposition:");
    if (!cd) return NULL;

    const char *fn_start = strstr(cd, "filename=\"");
    if (!fn_start) return NULL;

    fn_start += strlen("filename=\"");
    const char *fn_end = strchr(fn_start, '"');
    if (!fn_end) return NULL;

    size_t fn_len = fn_end - fn_start;
    char *filename = malloc(fn_len + 1);
    if (!filename) return NULL;

    strncpy(filename, fn_start, fn_len);
    filename[fn_len] = '\0';
    return filename;
}

char *extract_name(const char *header_block) {
    const char *cd = strstr(header_block, "Content-Disposition:");
    if (!cd) return NULL;

    const char *name_start = strstr(cd, "name=\"");
    if (!name_start) return NULL;

    name_start += strlen("name=\"");
    const char *name_end = strchr(name_start, '"');
    if (!name_end) return NULL;

    size_t name_len = name_end - name_start;
    char *name = malloc(name_len + 1);
    if (!name) return NULL;

    strncpy(name, name_start, name_len);
    name[name_len] = '\0';
    return name;
}

// Binary-safe memmem implementation
void *memmem_custom(const void *haystack, size_t haystacklen,
                    const void *needle, size_t needlelen) {
    if (needlelen == 0) return (void *)haystack;
    if (haystacklen < needlelen) return NULL;

    const char *h = (const char *)haystack;
    const char *n = (const char *)needle;

    for (size_t i = 0; i <= haystacklen - needlelen; i++) {
        if (memcmp(h + i, n, needlelen) == 0) {
            return (void *)(h + i);
        }
    }
    return NULL;
}

void parse_multipart_form_data(const char *body, size_t content_len, const char *boundary_string) {
    char boundary_start[256];
    char boundary_end[260];

    snprintf(boundary_start, sizeof(boundary_start), "--%s", boundary_string);
    snprintf(boundary_end, sizeof(boundary_end), "--%s--", boundary_string);

    size_t boundary_start_len = strlen(boundary_start);

    const char *cursor = body;
    size_t remaining = content_len;

    char savedFilename[256] = "\"image\":\"\"";
    char savedMessage[256];

    while (1) {
        // Find the next boundary
        char *part_start = memmem_custom(cursor, remaining, boundary_start, boundary_start_len);
        if (!part_start) break;  // No more parts

        part_start += boundary_start_len; // Skip boundary string

        // Check if end boundary
        if (remaining >= (size_t)(part_start - cursor + 2) && memcmp(part_start, "--", 2) == 0) {
            break;  // End of multipart
        }

        // Skip leading CRLF
        while ((size_t)(part_start - body) < content_len && (*part_start == '\r' || *part_start == '\n')) {
            part_start++;
        }

        size_t part_offset = part_start - body;
        remaining = content_len - part_offset;

        // Find end of headers (\r\n\r\n)
        char *header_end = memmem_custom(part_start, remaining, "\r\n\r\n", 4);
        if (!header_end) {
            fprintf(stderr, "Malformed part: no header end\n");
            break;
        }

        size_t headers_len = header_end - part_start;

        // Extract headers as a null-terminated string
        char *headers_str = malloc(headers_len + 1);
        if (!headers_str) {
            perror("malloc");
            break;
        }
        memcpy(headers_str, part_start, headers_len);
        headers_str[headers_len] = '\0';

        // Data start is after header end + 4 bytes for \r\n\r\n
        char *data_start = header_end + 4;

        // Find next boundary to determine data length
        char *next_boundary = memmem_custom(data_start, content_len - (data_start - body), boundary_start, boundary_start_len);

        size_t data_len;
        if (next_boundary) {
            data_len = next_boundary - data_start;
        } else {
            // Last part
            data_len = content_len - (data_start - body);
        }

        // Trim trailing CRLF from data
        while (data_len > 0 && (data_start[data_len - 1] == '\r' || data_start[data_len - 1] == '\n')) {
            data_len--;
        }

        // Print headers and data length for debugging
        printf("Headers:\n%s\n", headers_str);
        printf("Data length: %zu\n", data_len);

        // Extract filename if present
        char *filename = extract_filename(headers_str);
        char *name = extract_name(headers_str);
        char filepath[256];
        if (!(filename == NULL || filename[0] == '\0')) {
            printf("Filename: %s\n", filename);
            snprintf(filepath, sizeof(filepath), "uploads/%s", filename);
            // Example: save the file data to disk
            FILE *fp = fopen(filepath, "wb");
            if (fp) {
                fwrite(data_start, 1, data_len, fp);
                fclose(fp);
                printf("Saved file: %s\n", filename);
            } else {
                perror("Failed to open file for writing");
            }


            snprintf(savedFilename, sizeof(savedFilename), "\"image\":\"%s\"", filename);
            // printf("savedFilename:%s\n", savedFilename);

            free(filename);
        }else if(name){
		    char *value = malloc(data_len + 1);
		    if (value) {
		        memcpy(value, data_start, data_len);
		        value[data_len] = '\0'; // null terminate
		        printf("Field name: %s, value: %s\n", name, value);
		        strcpy(savedMessage, value);
		        // printf("saved message:%s\n", savedMessage);
		        // saveJSONToFile("uploads/received.json", value);
		        // Here, if name == "message", you can store or process the message text
		        free(value);
		    }
		    free(name);
        }

        free(headers_str);
        // Move cursor to next boundary start
        cursor = next_boundary;
        if (!cursor) break;  // No more parts

        remaining = content_len - (cursor - body);
    }

    char jsonString[256] = "{";
    deleteCharC(savedMessage, '{');
    // printf("saved msg:%s\n", savedMessage);
    strcat(jsonString, savedFilename);
    strcat(jsonString, ",");
    strcat(jsonString, savedMessage);
    printf("final json string:%s\n", jsonString);
    saveJSONToFile("uploads/received.json", jsonString);


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
		printf("[%s:%u]\n\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		
		//Reading the request
		//Create the method, path, and version variables
		char method[BUFFER_SIZE], uri[BUFFER_SIZE], version[BUFFER_SIZE];

		//the '%s' refers to the string formatting: each variable is formatted as a string
		sscanf(buffer, "%s %s %s", method, uri, version);

		//Print out the HTTP Request
		printf("%s %s %s\n", method, uri, version);

		if(strcmp(method, "POST") == 0 && strcmp(uri, "/api/chat") == 0){
			printf("Starting POST request\n");
			// printf("Printing buffer:\n%s\n\n", buffer);

			//Finding the content length:
			int content_len = 0;
			char *content_len_ptr = strstr(buffer, "Content-Length:");
			sscanf(content_len_ptr, "Content-Length: %d", &content_len);
			printf("content len:%d\n", content_len);

			//Finding the boundary string (haven't extracted the string yet):
			char *content_type_ptr = strstr(buffer, "Content-Type:");
			char content_type[128];
			char boundary_string[128];
			sscanf(content_type_ptr, "Content-Type: %127[^\r\n]", content_type);
			printf("content type string%s\n", content_type);

			char *boundary = strstr(content_type, "boundary=");
			strcpy(boundary_string, boundary + strlen("boundary="));


			boundary_string[strlen(boundary_string) - 1] = '\0';

		    // Trim trailing whitespace or newlines
		    size_t len = strlen(boundary_string);
		    while (len > 0 && (boundary_string[len - 1] == '\r' || boundary_string[len - 1] == '\n' || boundary_string[len - 1] == ' ')) {
		        boundary_string[len - 1] = '\0';
		        len--;
		    }

			printf("boundary:%s\n", boundary_string);

			// "\r\n\r\n" is the start of the header
			char *body_start = strstr(buffer, "\r\n\r\n");

			//Edge case
			if (!body_start){
				printf("Invalide POST request: missing body");
				close(newmysock);
				continue;
			}
			body_start += 4; //Move the point 4 elements ahead

			// printf("body start:\n%s\n\n", body_start);

			//Finding the number of elements/char in the body
			int header_len = body_start - buffer;
			int body_len = valread - header_len;

			//Assigning the body
			char *body = malloc(content_len + 1);
			memcpy(body, body_start, body_len);

			//If the body isn't complete, read the rest
			while(body_len < content_len){
				int bytes_read = read(newmysock, body + body_len, content_len - body_len);

				// returns -1 if it can't read anymore
				if (bytes_read <= 0){
					break;
				}
				body_len += bytes_read;
			}

			printf("body len:%d\n", body_len);
			if(body_len == content_len){
				printf("Body has been fully read!\n");
			}else{
				printf("Body has not been fully read!\n");
			}

			// printf("body:\n%s\n", body);
			parse_multipart_form_data(body, content_len, boundary_string);

			printf("multipart parsed!\n");
			const char *response_body = "{\"status\":\"success\", \"message\":\"File(s) uploaded and JSON saved.\"}";
			char response_header[BUFFER_SIZE];
			snprintf(response_header, sizeof(response_header),
			         "HTTP/1.1 200 OK\r\n"
			         "Content-Type: application/json\r\n"
			         "Content-Length: %zu\r\n"
			         "Connection: close\r\n\r\n",
			         strlen(response_body));

			// Send headers
			write(newmysock, response_header, strlen(response_header));
			// Send body
			write(newmysock, response_body, strlen(response_body));


			free(body);
			close(newmysock);
			continue;
		}


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
