#include "database.h"

//Client class
Client::Client(char user_id[USER_ID_MAX_LEN + 1], int sockfd){
	this->sockfd = sockfd;
	this->online = true;
	memset(this->user_id, 0, USER_ID_MAX_LEN + 1);
	memcpy(this->user_id ,user_id, USER_ID_MAX_LEN + 1);
}
char * Client::get_user_id(){
    return this->user_id;
}
bool Client::is_online(){
	return this->online;
}
void Client::set_online(){
	this->online = true;
}
void Client::set_offline(){
	this->online = false;
}
void Client::set_sockfd(int fd){
	this->sockfd = fd;
}
void Client::add_subscription(char topic[TOPIC_MAX_LEN], int SF){
	struct subscription subs;
	memset(&subs, 0, sizeof(subs));
	subs.SF = SF;
	memcpy(&subs.topic, topic, strlen(topic));
	remove_subscription(topic);
	subscriptions.push_back(subs);
	if(SF == 1){
		SF_subscriptions++;
	}
}
void Client::send_waiting_packets(){
	while(packet_queue.size() > 0){
		server_to_tcp_packet pck = packet_queue.front();
		packet_queue.erase(packet_queue.begin());
		int sendfd = send(sockfd, &pck, sizeof(pck), 0);
		DIE(sendfd < 0, "sending queue packets");
	}
}
bool Client::has_SF_subscriptions(){
	if(SF_subscriptions > 0){
		return true;
	}
	return false;
}
bool Client::has_subscriptions(){
	if(subscriptions.size() == 0){
		return false;
	}
	return true;
}
void Client::print_subscriptions(){
	for(int i = 0 ; i < subscriptions.size(); i++){
		printf("%s\n",subscriptions[i].topic);
	}
}
int Client::remove_subscription(char subscription[TOPIC_MAX_LEN]){
	for(int i = 0 ; i < subscriptions.size(); i++){
		if(strcmp(subscriptions[i].topic,subscription) == 0){
			if(subscriptions[i].SF == 1){
				SF_subscriptions--;
			}
			subscriptions.erase(subscriptions.begin() + i);
			return 1;
		}
	}
	return 0;
}
bool Client::has_subsciption_to(char *subscription){
	for(int i = 0 ; i < subscriptions.size(); i++){
		if(strcmp(subscriptions[i].topic,subscription) == 0){
			return true;
		}
	}
	return false;
}
void Client::add_packet_to_queue(server_to_tcp_packet pck){
	if(packet_queue.size() == MAXIMUM_WAITING_PACKETS){
		packet_queue.erase(packet_queue.begin());
	}
	packet_queue.push_back(pck);
}
int Client::get_sockfd(){
	return this->sockfd;
}



//ServerDataBase class

ServerDataBase::ServerDataBase(){
}
int ServerDataBase::get_nr_of_clients(){
	return tcp_client_list.size();
}
void ServerDataBase::add_client(Client *client){
    tcp_client_list.push_back(client);
}
void ServerDataBase::print_clients(){
	cout << "clients:\n";
	for(int i = 0; i < tcp_client_list.size(); i++){
		cout << tcp_client_list[i]->get_user_id() << " " <<
				tcp_client_list[i]->get_sockfd() << endl;
	}
}
Client* ServerDataBase::get_client_by_user_id(char user_id[USER_ID_MAX_LEN + 1]){
    for(int i = 0; i < tcp_client_list.size(); i++){
        if(strcmp(tcp_client_list[i]->get_user_id(), user_id) == 0 ){
            return tcp_client_list[i];
        }
    }
    return NULL;
}
Client* ServerDataBase::get_client_by_sockfd(int sockfd){
	for(int i = 0; i < tcp_client_list.size(); i++){
        if(tcp_client_list[i]->get_sockfd() ==  sockfd){
            return tcp_client_list[i];
        }
    }
    return NULL;
}
void ServerDataBase::send_subscription_packet_to_tcp_clients(server_to_tcp_packet tcp_pck) {
	for(int i = 0; i < tcp_client_list.size(); i++) {
		if(tcp_client_list[i]->has_subsciption_to(tcp_pck.topic)) {
			//the client has subscription to this topic
			if(tcp_client_list[i]->is_online()){
				//send the packet to the online clients
				int sendfd = send(tcp_client_list[i]->get_sockfd(), &tcp_pck, sizeof(tcp_pck), 0);
				if(sendfd == 0){
					fprintf(stderr, "Error. Sending the packet to tcp client failed\n");
				}
			}else if(tcp_client_list[i]->has_SF_subscriptions()){
				//client has Store&Forward subscription
				tcp_client_list[i]->add_packet_to_queue(tcp_pck);
			}
		}
    }
}
void ServerDataBase::send_exit_packet_to_all_clients(server_to_tcp_packet pck){
	for(int i = 0; i < tcp_client_list.size(); i++) {
		int sendfd = send(tcp_client_list[i]->get_sockfd(), &pck, sizeof(pck), 0);
		if(sendfd == 0){
			fprintf(stderr, "Error. Sending the packet to tcp client failed\n");
		}
        close(tcp_client_list[i]->get_sockfd());
	}
}
void ServerDataBase::print_subs(){
	for(int i = 0; i < tcp_client_list.size(); i++){
		tcp_client_list[i]->print_subscriptions();
	}
}
int ServerDataBase::add_subscription(tcp_to_server_packet pck){
	Client *client = get_client_by_user_id(pck.user_id);
	if(client == NULL){
		return -1;
	}
	client->add_subscription(pck.topic, pck.SF);
	return 1;
}
int ServerDataBase::remove_subscription(tcp_to_server_packet pck){
	Client *client = get_client_by_user_id(pck.user_id);
	if(client == NULL){
		return -1;
	}
	return client->remove_subscription(pck.topic);
}
bool ServerDataBase::has_client_with_sockfd(int sockfd){
	for(int i = 0; i < tcp_client_list.size(); i++){
		if(tcp_client_list[i]->get_sockfd() == sockfd){
			return true;
		}
	}
	return false;
}
int ServerDataBase::remove_client(char id[USER_ID_MAX_LEN + 1]){
	for(int i = 0; i < tcp_client_list.size(); i++){
		if(strcmp(tcp_client_list[i]->get_user_id(), id) == 0){
			tcp_client_list.erase(tcp_client_list.begin() + i);
			return 1;
		}
	}
	return -1;
}
