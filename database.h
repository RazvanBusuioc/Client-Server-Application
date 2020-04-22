#ifndef DATABASE_H_
#define DATABASE_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "math.h"   
#include <iostream>
#include <vector>

using namespace std;

class Client{
    private:
		//queue for storing the subscription packets for clients with SF subscription
        vector<server_to_tcp_packet> packet_queue;

		//list of subscriptions
		vector<struct subscription> subscriptions;

		//the socket file descriptor that connects the server to the client
		int sockfd;

		//number of SF subscriptions
		int SF_subscriptions = 0;

		//the user id
        char user_id[USER_ID_MAX_LEN + 1];

		//online/offline status
        bool online;

    public:
        Client(char user_id[USER_ID_MAX_LEN + 1], int sockfd);

		//returns the socket fd of the client
		int get_sockfd();

		//removes the subscription given as param from the subscription list
		int remove_subscription(char subscription[TOPIC_MAX_LEN]);

		//returns the user id
        char *get_user_id();

		//prints all the subscriptions
		void print_subscriptions();

		//sets the client online
		void set_online();

		//sets the client offline
		void set_offline();

		//sets a new sockfd. Used when a client reconnects
		void set_sockfd(int fd);
		
		//adds a new subscription to the list
		void add_subscription(char topic[TOPIC_MAX_LEN], int SF);

		//add a new subscription packet to the waiting queue. Used when a client has
		//SF subscription but he is offline
		void add_packet_to_queue(server_to_tcp_packet pck);

		//sends the packet in the queue. Used when a client with SF subscription recconects
		void send_waiting_packets();
		
		//true if nr of SF subscriptions > 0
		bool has_SF_subscriptions();

		//true if client has any subscriptions
		bool has_subscriptions();

		bool is_online();

		bool has_subsciption_to(char *subscription);
};

class ServerDataBase{
    private:
		//list of clients
        vector<Client *> tcp_client_list;

    public:
		ServerDataBase();

		//returns a pointer to the client with given user id
		Client* get_client_by_user_id(char user_id[USER_ID_MAX_LEN + 1]);

		//returns a pointe to the client with given sockfd
		//Used when a client disconnects unexpectedly(without announcing the server)
		Client* get_client_by_sockfd(int sockfd);

		//returns the number of clients in the database
		int get_nr_of_clients();

		//unsubscribe to the topic in 'pck.topic'
		int remove_subscription(tcp_to_server_packet pck);

		//removes the client with given user id from the data base
		int remove_client(char id[USER_ID_MAX_LEN + 1]);

		//subscribe to the topic in 'pck.topic'
		int add_subscription(tcp_to_server_packet pck);
		
		//adds a new client to the database
        void add_client(Client *client);

		void print_clients();

		//sends the packet to the online clients and stores it in the waiting queue
		//for the offline clients that have a SF subscription
		void send_subscription_packet_to_tcp_clients(server_to_tcp_packet tcp_pck);

		//used when the server closed, sends a packet to let the client know
		//that the server closed. The function also closes all sockets for client communication
		void send_exit_packet_to_all_clients(server_to_tcp_packet pck);

		//prints the subscriptions of all clients
		void print_subs();

		bool has_client_with_sockfd(int sockfd);
};

#endif