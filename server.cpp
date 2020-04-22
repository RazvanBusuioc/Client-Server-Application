#include "database.h"

using namespace std;

//creates a packet in order to send it to tcp clients
server_to_tcp_packet create_server_to_tcp_packet
(struct sockaddr_in udp_client, struct udp_to_server_packet udp_pck) {

	server_to_tcp_packet pck;
	char *aux_ip;
	memset(&pck, 0, sizeof(pck));

	//filing the data in the packet
	aux_ip = (char *)malloc(15);
	aux_ip = inet_ntoa(udp_client.sin_addr);
	pck.type = 0;
	pck.port = udp_client.sin_port;
	pck.data_type = udp_pck.payload_type;
	memcpy(&pck.udp_ip, aux_ip, strlen(aux_ip));
	memcpy(&pck.topic, &udp_pck.topic, strlen(udp_pck.topic));
	if(pck.data_type == 0){
		struct INT *aux = (struct INT*)&udp_pck.payload;
		memcpy(&pck.payload, aux, sizeof(struct INT));
	}
	else if(pck.data_type == 1){
		struct SHORT_REAL *aux = (struct SHORT_REAL*)&udp_pck.payload;
		memcpy(&pck.payload, aux, sizeof(struct SHORT_REAL));
	}
	else if(pck.data_type == 2){
		struct FLOAT *aux = (struct FLOAT*)&udp_pck.payload;
		memcpy(&pck.payload, aux, sizeof(struct FLOAT));
	}
	else if(pck.data_type == 3){
		struct STRING *aux = (struct STRING*)&udp_pck.payload;
		memcpy(&pck.payload, &udp_pck.payload, strlen(aux->string));
	}
	return pck;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s server_port\n", argv[0]);
		exit(0);
	}
	ServerDataBase *server_db = new ServerDataBase();

	//variables used for file descriptors/return values
	int tcp_sockfd, newsockfd, server_port, n, i, ret;
	//buffer used for reading from stdin
	char buffer[BUFLEN];
	//structures used for socket calls
	struct sockaddr_in serv_addr, cli_addr;
	struct sockaddr_in udp_sockaddr, udp_client ;
	socklen_t clilen;

	// select reading file descriptors
	fd_set read_fds;
	// select temporary file descriptors	
	fd_set tmp_fds;
	// maxim value of file descriptors
	int fdmax;			   	

	//open socket for listening to tcp packets
	tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sockfd < 0, "socket");

	//set the value of the port given as parameter in command line
	server_port = atoi(argv[1]);
	DIE(server_port == 0, "atoi");

	//setting the sockaddr struct for tcp binding
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(server_port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	//binding
	ret = bind(tcp_sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");
	
	//listening for a connection with tcp clients to accept
	int listenfd = listen(tcp_sockfd, MAX_CLIENTS);
	DIE(listenfd < 0, "listen");

    ///open socket for listening to udp clients
	int udp_sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(udp_sockfd < 0, "socket");

	//set the sockaddr struct for udp binding
	udp_sockaddr.sin_family = AF_INET;
	udp_sockaddr.sin_port = htons(atoi(argv[1]));
	udp_sockaddr.sin_addr.s_addr = INADDR_ANY;

    int rs = bind(udp_sockfd, (struct sockaddr *)&udp_sockaddr, sizeof(udp_sockaddr));
	DIE(rs < 0, "bind");

	//clearing and setting the file descriptors
    FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
	FD_SET(STDIN, &read_fds);
	FD_SET(udp_sockfd, &read_fds);
	FD_SET(tcp_sockfd, &read_fds);
	fdmax = udp_sockfd;
 
	while (1) {
		tmp_fds = read_fds;
		//select the file descriptors to read from
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == STDIN)	{
					//read from stdin
					char buffer[COMMAND_MAX_LEN];
					memset(buffer, 0, COMMAND_MAX_LEN);
					fgets(buffer, BUFLEN - 1, stdin);

					if(strcmp(buffer, "exit\n") == 0){
						//exit command
						//build exit packet and send it to tcp clients
						server_to_tcp_packet pck;
						pck.type = 1;
						server_db->send_exit_packet_to_all_clients(pck);
						sleep(0.2);

						//close the server
						close(tcp_sockfd);
    					close(udp_sockfd);
						return 0;
					}else{
						//wrong command at stdin
						printf("No such command %s", buffer);
					}
				}
				else if (i == tcp_sockfd) {
					//server received a connection request from a tcp client

					//accept the connection
					clilen = sizeof(cli_addr);
					newsockfd = accept(tcp_sockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");
					
					//receive the id of the client that just connected
					char buf[USER_ID_MAX_LEN + 1];
					memset(&buf, 0,USER_ID_MAX_LEN + 1);
					int n = recv(newsockfd, buf, USER_ID_MAX_LEN + 1, 0);
					DIE(n < 0, "client connection");

					//add the file descriptor to the read fds
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}

					if(server_db->get_client_by_user_id(buf) != NULL){
						if(server_db->get_client_by_user_id(buf)->is_online() == true){
							//the user that wants a connection already exists and is online
							//an invalid connection

							//send the client an invalidation for the connection
							//reason: user name already online
							int validation = 0;
							n = send(newsockfd, &validation , sizeof(int), 0);
							DIE(send < 0, "client connection");

							//erase this file descriptor
							FD_CLR(newsockfd, &read_fds);

							//close the socket for this connection
							close(newsockfd);
						}else{
							//the user that wants a connection already exists in the database
							//but he is not online. A valid connection

							//send the client a validation for the connection
							int validation = 1;
							n = send(newsockfd, &validation , sizeof(int), 0);
							DIE(send < 0, "client connection");

							printf("New client (%s) connected from %s:%d\n", buf, 
								inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

							//set the client status to online
							server_db->get_client_by_user_id(buf)->set_online();

							//set the new socket file descriptor
							server_db->get_client_by_user_id(buf)->set_sockfd(newsockfd);

							//send any waiting packets for the SF subscription(s)
							server_db->get_client_by_user_id(buf)->send_waiting_packets();
						}
					}
					else{
						//a new and valid connection
						printf("New client (%s) connected from %s:%d\n", buf, 
								inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

						//add the client in the database
						server_db->add_client(new Client(buf, newsockfd));

						//send the client a validation for the new connection
						int validation = 1;
						n = send(newsockfd, &validation , sizeof(int), 0);
						DIE(send < 0, "client connection");

						
					}
				}
				else if (i == udp_sockfd){
					//server received a packet from an udp client

					//setting the structures for receiving and udp packet
					int bytesread;
					udp_to_server_packet udp_pck;
					uint32_t len = sizeof(udp_client);
					memset(&udp_pck, 0, sizeof(udp_pck));

					//receive an udp packet
					bytesread = recvfrom(udp_sockfd, &udp_pck, UDP_PCK_MAX_LEN ,
									0, (struct sockaddr*)&udp_client, &len);//)){
										 
					//read the data from the received packet and create
					//a new packet in order to send it to tcp clients					
					server_to_tcp_packet pck = 
						create_server_to_tcp_packet(udp_client, udp_pck);

					//send the packet to it`s clients
					server_db->send_subscription_packet_to_tcp_clients(pck);
				}
				else if(server_db->has_client_with_sockfd(i)){
					//there is  packet to receive from a tcp client
					struct tcp_to_server_packet received_packet;
					memset(&received_packet, 0, sizeof(received_packet));

					//receive the packet
					n = recv(i, &received_packet, sizeof(received_packet), 0);
					DIE(n < 0, "recv");

					if(n == 0){
						//client has closed the connection unexpectedly
						printf("Client (%s) disconnected\n", 
								server_db->get_client_by_sockfd(i)->get_user_id());

						if(server_db->get_client_by_sockfd(i)->
							has_subscriptions() == false){
							//the user that is disconnecting does not have any subscription
							//so he/she will be removed from the database
							server_db->remove_client(server_db->get_client_by_sockfd(i)->get_user_id());
						}
						else{
							//set the client offline
							server_db->get_client_by_sockfd(i)->set_offline();

							//modify sockfd so it would not interfeer in other actions
							server_db->get_client_by_sockfd(i)->set_sockfd(-1);
						}

						//clear the client`s file descriptor
						FD_CLR(i, &read_fds);

						//close the connection
						close(i);
						continue;
					}

					if (received_packet.type == -1) {
						//the tcp client closed connection
						printf("Client (%s) disconnected\n", received_packet.user_id);

						if(server_db->get_client_by_user_id(received_packet.user_id)->
							has_subscriptions() == false){
							//the user that is disconnecting does not have any subscription
							//so he/she will be removed from the database
							server_db->remove_client(received_packet.user_id);
						}
						else{
							//set the client offline
							server_db->get_client_by_user_id(received_packet.user_id)->set_offline();

							//modify sockfd so it would not interfeer in other actions
							server_db->get_client_by_user_id(received_packet.user_id)->set_sockfd(-1);
						}
						//close the connection
						close(i);
						
						//erase client`s file descriptor
						FD_CLR(i, &read_fds);
					}
					else if(received_packet.type == 1){
						//the tcp client has requested a subscription
						if (server_db->add_subscription(received_packet) == -1){
							//the user id is not found
							printf("nu such user found: %s", received_packet.user_id);
						}
						//server_db->print_subs();
					}
					else if(received_packet.type == 0){
						//the tcp client has requested an unsubscription
						int n = server_db->remove_subscription(received_packet);
						if(n == -1){
							//the client id has not been found
							printf("no client found\n");
						}else if(n == 0){
							//the topic was not found
							printf("no such subscription found\n");
						}
					}
				}
			}
		}
	}

	close(tcp_sockfd);
    close(udp_sockfd);
	return 0;
}
