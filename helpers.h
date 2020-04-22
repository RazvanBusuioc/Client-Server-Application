#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

//max dimension for stdin commands
#define BUFLEN		256	

//max number of clients that a server cand connect with
#define MAX_CLIENTS	10000

//max dimension for a string message
#define MAX_STRING_LEN 1500
#define STDIN 0

//topic len for a server to tcp packet
#define TOPIC_MAX_LEN 51

//topic max len for a udp to server packet
#define UDP_TOPIC_MAX_LEN 50

//maximum number of chars for a stdin command
#define COMMAND_MAX_LEN 50

//maximum bytes to read when receiving an UDP packet
#define UDP_PCK_MAX_LEN 1600

//maximum waiting packets in a client queueu
#define MAXIMUM_WAITING_PACKETS 1000

//used when a client types a wrong command at stdin
#define TCP_CLIENT_COMMAND_USAGE "Command not found. Did you mean:\n\tsubscribe {topic} 0\n\tsubscribe {topic} 1\n\tunsubscribe {topic}\n\texit\n"

#define USER_ID_MAX_LEN 10

//used for sending (un)subscription packets to the server
struct tcp_to_server_packet
{
	char user_id[USER_ID_MAX_LEN + 1];
	int type; //subscribe = 1 / unsubscribe = 0 / exit = -1
	char topic[TOPIC_MAX_LEN]; //topic of (un)subscription
	int SF; //sotre&forward
}__attribute__ ((__packed__));

//used for the list of subscription in the client class
struct subscription
{
	char topic[TOPIC_MAX_LEN];
	int SF;
};

//used for receiving the data from the udp clients
struct udp_to_server_packet
{
	char topic[UDP_TOPIC_MAX_LEN];
	uint8_t payload_type;
	char payload[MAX_STRING_LEN];
}__attribute__ ((__packed__));

//used for sending packets to the tcp clients
struct server_to_tcp_packet
{
	uint8_t type; // 0 = normal packet /1 = server closed
	char udp_ip[15]; 
	uint16_t port;
	char topic[TOPIC_MAX_LEN];
	uint8_t data_type;
	char payload[MAX_STRING_LEN];
}__attribute__ ((__packed__));

//data type 0
struct INT
{
	uint8_t sign;
	uint32_t number;
}__attribute__ ((__packed__));

//data type 1
struct SHORT_REAL
{
	uint16_t number;
}__attribute__ ((__packed__)) ;

//data type 2
struct FLOAT
{
	uint8_t sign;
	uint32_t merged_number;
	uint8_t pow_of_ten;
}__attribute__ ((__packed__));

//data type 3
struct STRING
{
	char string[MAX_STRING_LEN];
}__attribute__ ((__packed__));

#endif
