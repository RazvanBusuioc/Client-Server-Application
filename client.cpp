#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include "helpers.h"
#include "math.h"   

using namespace std;

//function that separes the arguments of a given command
char **separate_stdin_command(char aux_buffer[BUFLEN - 1]){
	char **fields = (char**)malloc(4 * BUFLEN);
	memset(fields, 0,4 * BUFLEN );
	char delim[] = " ";
	char *ptr = strtok(aux_buffer, delim);
	int i = 0;
	while (ptr != NULL){
		fields[i++] = ptr;
		ptr = strtok(NULL, delim);
	}
	return fields;
}


int main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Usage: %s id_client ip_server server_port\n", argv[0]);
		exit(0);
	}
	if(strlen(argv[1]) > USER_ID_MAX_LEN){
		fprintf(stderr, "Usage: user id %s is to long. Try using an id with len <= 10\n", argv[1]);
		exit(0);
	}

	//variables used for file descriptors/return values
	int sockfd, n, ret;
	//buffer user for reading from stdin
	char aux_buffer[BUFLEN];
	//structure used for socket calls
	struct sockaddr_in serv_addr;
	//packet for storing the received data
	struct server_to_tcp_packet received_packet;
	//subscription packet that needs to be sent to server
	struct tcp_to_server_packet subscription_packet;
	//reading file descriptors sets
	fd_set read_fds;
	FD_ZERO(&read_fds);
	
	//open the socket for tcp connection
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");
	
	//setting the sockaddr struct for tcp connection
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));	
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	//connects the client to the server
	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");
	
	//send my user id to the server
	DIE(send(sockfd, argv[1], USER_ID_MAX_LEN + 1, 0) < 0, "send");

	//receiving a validation regardin the user id from the server
	int validation;
	n = recv(sockfd, &validation, sizeof(validation), 0);
	DIE(n < 0, "receiving validation for user id");

	if(validation == 0){
		//user id already in use(someone online with the same user)
		fprintf(stderr,"UserID %s already in use. Please pick another userID\n", argv[1]); 
		close(sockfd);
		return 0;
	}
	
	while (1) {

		//set the 2 file descriptors and select
		FD_SET(0, &read_fds);
		FD_SET(sockfd, &read_fds);
		ret = select(sockfd + 1 ,&read_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		if(FD_ISSET(STDIN,&read_fds)){
			//reading from stdin

	  		//clear the memory of the auxiliary packets
			memset(&subscription_packet, 0, sizeof(struct tcp_to_server_packet));
			memset(&subscription_packet.topic, 0, TOPIC_MAX_LEN);

			//fill the user id in the packet to be sent
			memcpy(&subscription_packet.user_id, argv[1], strlen(argv[1]));

			//read from stdin in an auxiliary buffer
			fgets(aux_buffer, BUFLEN - 1, stdin);

			//separate the buffer into command + arguments 
			char **fields = (char **)malloc(4 * BUFLEN);
			fields = separate_stdin_command(aux_buffer);
			
			if (strcmp(fields[0], "exit\n") == 0) {
				//exit command
				if(fields[1] != NULL){
					//more arguments than needed
					fprintf(stderr,TCP_CLIENT_COMMAND_USAGE); 
				}
				else{
					//send packet to in order to sign the server that a cliend disconnected
					subscription_packet.type = -1;
					memcpy(&subscription_packet.user_id, argv[1], strlen(argv[1]));
					int fd = send(sockfd, &subscription_packet, sizeof(subscription_packet), 0);
					DIE(fd < 0, "send");
					break;
				}
			}
			else if (strcmp(fields[0], "subscribe") == 0){
				//subscribe command
				if(fields[3] != NULL){
					//more arguments than needed
					fprintf(stderr,TCP_CLIENT_COMMAND_USAGE); 
				}
				else if(fields[2] == NULL){
					//only 1 argument prvided (subscribe topic)
					fprintf(stderr,TCP_CLIENT_COMMAND_USAGE); 
				}
				else if (fields[2][0] != '0' && fields[2][0] != '1'){
					//2nd argument is invalid
					fprintf(stderr,TCP_CLIENT_COMMAND_USAGE); 
				}else{
					//send subscription message to server

					//fill the data in the packet
					subscription_packet.type = 1;
					memcpy(&subscription_packet.topic, fields[1], strlen(fields[1]));
					subscription_packet.SF = atoi(fields[2]);
					if(strlen(fields[1]) == TOPIC_MAX_LEN - 1){
						//case of a 50 len topic
						strcat(subscription_packet.topic, "\0");
					}
					int fd = send(sockfd, &subscription_packet, sizeof(subscription_packet), 0);
					DIE(fd < 0, "send");
					printf("Subscribed %s\n", fields[1]);
				}
			}
			else if (strcmp(fields[0], "unsubscribe") == 0){
				//unsubscribe command
				if(fields[2] != NULL){
					//more arguments than needed
					fprintf(stderr,TCP_CLIENT_COMMAND_USAGE); 
				}
				else{
					//send unsubscription message to server
					subscription_packet.type = 0;
					memcpy(&subscription_packet.topic, fields[1], strlen(fields[1]) - 1);
					if(strlen(fields[1]) == TOPIC_MAX_LEN - 1){
						//case of a 50 len topic
						strcat(subscription_packet.topic, "\0");
					}
					int fd = send(sockfd, &subscription_packet, sizeof(subscription_packet), 0);
					DIE(fd < 0, "send");
					printf("Unsubscribed %s", fields[1]);
				}
			}
			else{
				//invalid command
				fprintf(stderr, TCP_CLIENT_COMMAND_USAGE);
			}
		}
		else if (FD_ISSET(sockfd, &read_fds)){
			//reading from socket
			//clear memory of the auxiliary packet
			memset(&received_packet, 0, sizeof(struct server_to_tcp_packet));

			//receive packet
			int m = recv(sockfd, &received_packet, sizeof(struct server_to_tcp_packet), 0);
			DIE(m < 0, "receive");

			if(received_packet.type == 1 || m == 0){
				//server closed (m == 0 => server closed unexpectedly)
				break;
			}else{
				//the client has received a subscription message
				if(received_packet.data_type == 0){
					//payload type is integer
					struct INT *integer = (struct INT*)&received_packet.payload;
					int32_t aux = integer->sign == 1 ? (-1 * ntohl(integer->number)) 
													: ntohl(integer->number);

					printf("%s:%d - %s - %s - %d\n", received_packet.udp_ip,
													received_packet.port,
													received_packet.topic,
													"INT", aux);
				}
				else if(received_packet.data_type == 1){
					//payload type is 2 decimals positive real number
					struct SHORT_REAL *real = (struct SHORT_REAL*)&received_packet.payload;
					double aux = (double)((double)ntohs(real->number) / (double)100);
					printf("%s:%d - %s - %s - %.2lf\n", received_packet.udp_ip,
													received_packet.port,
													received_packet.topic,
													"SHORT_REAL", aux);
				}
				else if(received_packet.data_type == 2){
					//payload type is float
					struct FLOAT *flt = (struct FLOAT*)&received_packet.payload;
					float aux = (float)((float)ntohl(flt->merged_number) /
								(float)pow((double)10, (double)flt->pow_of_ten));
					aux = flt->sign == 1 ? (-1 * aux) : aux;
					printf("%s:%d - %s - %s - %lf\n", received_packet.udp_ip,
													received_packet.port,
													received_packet.topic,
													"FLOAT", aux);
				}
				else if(received_packet.data_type == 3){
					//payload type is string
					struct STRING *str = (struct STRING*)&received_packet.payload;
					//printf("len is %d\n", sizeof(void *));
					printf("%s:%d - %s - %s - %s\n", received_packet.udp_ip,
													received_packet.port,
													received_packet.topic,
													"STRING", str->string);
				}
				else{
					fprintf(stderr,"Packet received from server is corrupted/does not match the protocol\n"); 
				}
			}
		}
	}
	close(sockfd);
	return 0;
}
