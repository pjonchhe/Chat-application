#ifndef COMMON_H_
#define COMMON_H_

#include "global.h"

struct client_details
{
	int port;
	bool bLoggedIn;
	int sock_fd;
	char ip_addr[HOSTNAME_LEN];
	int num_msg_sent;
	int num_msg_rcv;
	int num_blocked;
	int blocked_clients[MAX_CLIENTS];
};

void printIP();
int getListenPort();
void setListenPort(int port);
void setClientPort(int port, int index);
int getClientPort(int index);
void printAuthor();
void addClient(char * client_IP, int fd, int port);
int getNumClients();
bool isClientLoggedIn(int index);
void setClientLoggedIn(int index, bool status);
void removeClient(int fd);
void clearFromBlockedList(int fd);
void incrementSentMsgCnt(int index);
void incrementRecvdMsgCnt(int index);
void blockClientWithfd(int block_fd, int client_index);
void unblockClientWithfd(int block_fd, int client_index);
int findClientfd(char *ip);
void getClientIPFromfd(int fd, char* IP);
int getClientfdFromIndex(int index);
int getClientIndexFromfd(int fd);
int getClientIndexFromIP(char* IP);
void printList();
bool validateIP(char *IP);
void getList(struct client_details* client_list);
void parseClientList(char *data);
void printStatistics();
void printBlockedClientList(char* client_IP);
char * tokenize(char * dest, char * src, char *delim);
bool isBlocked(int client_index, int blocked_fd);


#endif
