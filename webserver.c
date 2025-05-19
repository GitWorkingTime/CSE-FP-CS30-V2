#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#define PORT 8080
#define BUFFER_SIZE 1024

/*
This local web server can be accessed via:
	http://localhost:8080
*/

int main() {
	char buffer[BUFFER_SIZE];
	char resp[] = "HTTP/1.0 200 OK\r\n"
				  "Server: webserver-c\r\n"
				  "Content-type: text/html\r\n\r\n"
				  "<html> WEBSERVER IS UP AND RUNNING WOO </html>\r\n";

	//Create socket
	int mysock = socket(AF_INET, SOCK_STREAM, 0);
	if (mysock == -1){
		perror("webserver (socket)");
		return 1;
	}
	printf("socket created successfully woooo\n");
	printf("\n");

	//Create the address for the socket to bind onto
	struct sockaddr_in host_addr;
	int host_addrlen = sizeof(host_addr);

	//Setting the address properties
	host_addr.sin_family = AF_INET;
	host_addr.sin_port = htons(PORT);
	host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//Curious as to what the data assigned is
	printf("host_addr.sin_family: \t %d\n", host_addr.sin_family);
	printf("host_addr.sin_port: \t %d\n", host_addr.sin_port);
	printf("host_addr.sin_addr.s_addr: \t %d\n", host_addr.sin_addr.s_addr);
	printf("host IP address and Port \t [%s:%u]\n", inet_ntoa(host_addr.sin_addr), ntohs(host_addr.sin_port));
	printf("\n");


	//Create the client address
	struct sockaddr_in client_addr;
	int client_addrlen = sizeof(client_addr);

	//Binding the socket to the address
	// The if function serves to check for an error (i.e if the bind function returns a -1)
	if (bind(mysock, (struct sockaddr *)&host_addr, host_addrlen) != 0){
		perror("webserver (bind");
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

		//Curious as to what newmysock value is:
		// printf("newmysock value: \t %d\n", newmysock);

		//If the newmysock returns an error(a.k.a -1)
		if(newmysock < 0){
			perror("webserver (accept)");

			//Go to the next socket queued
			continue; 
		}
		printf("connnection accepted\n");

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

		//Printing the request header
		printf("%s %s %s\n", 
			   method,
			   version,
			   uri);

		printf("\n");

		//Writing to the socket
		int valwrite = write(newmysock, resp, strlen(resp));

		//If there is an error, go to the next socket queued up
		if (valwrite < 0){
			perror("webserver (write");
			continue;
		}

		close(newmysock);
	}
	return 0;
}