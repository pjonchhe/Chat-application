#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>


#include "../include/common.h"
#include "../include/logger.h"
#include "../include/global.h"
using namespace std;

int g_listenPort = -1;
int num_clients = 0;
//struct sockaddr_in client_list[MAX_CLIENTS];
struct client_details client_l[MAX_CLIENTS];

void printIP()
{
	int dummy_socket;
	char str[128];

	dummy_socket = socket(AF_INET, SOCK_DGRAM, 0);
	
	struct sockaddr_in server_address, local_address;
	socklen_t  len = sizeof(struct sockaddr);

        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(53);
        inet_pton(AF_INET, "8.8.8.8", &server_address.sin_addr.s_addr);

	int connection_status = connect(dummy_socket, (struct sockaddr *) &server_address, sizeof(server_address));
	if(connection_status == -1)
	{
		printf("There was an error\n");
	}
	
	getsockname(dummy_socket, (struct sockaddr *)&local_address, &len);

	inet_ntop(AF_INET, &local_address.sin_addr, str, sizeof(str));

	cse4589_print_and_log("IP:%s\n", str);
}

int getListenPort()
{
	return g_listenPort;
}

void setListenPort(int port)
{
	g_listenPort = port;
}

void setClientPort(int port, int index)
{
	client_l[index].port = port;
}

int getClientPort(int index)
{
	return client_l[index].port;
}

void printAuthor()
{
	cse4589_print_and_log("I, prajinjo, have read and understood the course academic integrity policy.\n");
}

//void addClient(struct sockaddr_in client_addr, int fd)
void addClient(char * client_IP, int fd, int port)
{
	char str[128];
	if(num_clients < MAX_CLIENTS)
	{
		//if((num_clients == 0) || (client_l[num_clients - 1].port < client_addr.sin_port))
		if((num_clients == 0) || (client_l[num_clients - 1].port < port))
		{
			//client_l[num_clients].port = client_addr.sin_port;
			client_l[num_clients].port = port;
			client_l[num_clients].bLoggedIn = true;
			client_l[num_clients].sock_fd = fd;
			//inet_ntop(AF_INET, &(client_addr.sin_addr), client_l[num_clients].ip_addr, sizeof(client_l[num_clients].ip_addr));
			strcpy(client_l[num_clients].ip_addr, client_IP);
			client_l[num_clients].num_msg_sent = 0;
			client_l[num_clients].num_msg_rcv = 0;
			client_l[num_clients].num_blocked = 0;
			for(int k = 0; k < MAX_CLIENTS; k++)
				client_l[num_clients].blocked_clients[k] = 0;
			num_clients++;
		}
		else
		{
			for(int i = 0; i < num_clients; i++)
			{
				//if(client_l[i].port < client_addr.sin_port)
				if(client_l[i].port < port)
					continue;
				else
				{
					for(int j = num_clients - 1; j >= i; j--)
					{
						client_l[j + 1].port = client_l[j].port;
						client_l[j + 1].bLoggedIn = client_l[j].bLoggedIn;
						client_l[j + 1].sock_fd = client_l[j].sock_fd;
						strcpy(client_l[j + 1].ip_addr, client_l[j].ip_addr);
						client_l[j + 1].num_msg_sent = client_l[j].num_msg_sent;
						client_l[j + 1].num_msg_rcv = client_l[j].num_msg_rcv;
						client_l[j + 1].num_blocked = client_l[j].num_blocked;
						for(int k = 0; k < MAX_CLIENTS; k++)
							client_l[j + 1].blocked_clients[k] = client_l[j].blocked_clients[k];
					}
					//inet_ntop(AF_INET, &(client_addr.sin_addr), client_l[i].ip_addr, sizeof(client_l[i].ip_addr));
					strcpy(client_l[i].ip_addr, client_IP);
					//client_l[i].port = client_addr.sin_port;
					client_l[i].port = port;
					client_l[i].bLoggedIn = true;
					client_l[i].sock_fd = fd;
					client_l[i].num_msg_sent = 0;
					client_l[i].num_msg_rcv = 0;
					client_l[i].num_blocked = 0;
					for(int k = 0; k < MAX_CLIENTS; k++)
						client_l[i].blocked_clients[k] = 0;
					num_clients++;
					break;
				}
			}
		}
	}
}

int getNumClients()
{
	return num_clients;
}

bool isClientLoggedIn(int index)
{
	return client_l[index].bLoggedIn;
}

void setClientLoggedIn(int index, bool status)
{
	client_l[index].bLoggedIn = status;
}

void removeClient(int fd)
{
	for(int i = 0; i < num_clients; i++)
	{
		if(client_l[i].sock_fd == fd)
		{
			for(int j = i; j < num_clients - 1; j++)
			{
				strcpy(client_l[j].ip_addr, client_l[j + 1].ip_addr);
				client_l[j].port = client_l[j + 1].port;
				client_l[j].bLoggedIn = client_l[j + 1].bLoggedIn;
				client_l[j].sock_fd = client_l[j + 1].sock_fd;
				client_l[j].num_msg_sent = client_l[j + 1].num_msg_sent;
				client_l[j].num_msg_rcv = client_l[j + 1].num_msg_rcv;
				client_l[j].num_blocked = client_l[j + 1].num_blocked;
				for(int k = 0; k < MAX_CLIENTS; k++)
				{
					client_l[j].blocked_clients[k] = client_l[j + 1].blocked_clients[k];
				}
			}
			num_clients--;
			break;
		}
			
	}
}

void clearFromBlockedList(int fd)
{
	for(int i = 0; i < num_clients; i++)
	{
		for(int j = 0; j < MAX_CLIENTS; j++)
		{
			if(fd == client_l[i].blocked_clients[j])
			{
				int num = client_l[i].num_blocked;
				for(int k = j; k < num - 1; k++)
				{
					client_l[i].blocked_clients[k] = client_l[i].blocked_clients[k + 1];
				}
				client_l[i].blocked_clients[num - 1] = 0;
				 client_l[i].num_blocked --;
			}
		}
	}
}

void incrementSentMsgCnt(int index)
{
	client_l[index].num_msg_sent++;
}

void incrementRecvdMsgCnt(int index)
{
	client_l[index].num_msg_rcv++;
}

void blockClientWithfd(int block_fd, int client_index)
{
	client_l[client_index].blocked_clients[client_l[client_index].num_blocked] = block_fd;
	client_l[client_index].num_blocked++;
}

void unblockClientWithfd(int block_fd, int client_index)
{
	for(int i = 0; i < client_l[client_index].num_blocked; i++)
	{
		if(client_l[client_index].blocked_clients[i] != block_fd)
			continue;
		else
		{
			int j;
			for(j = i; j < client_l[client_index].num_blocked - 1; j++)
				client_l[client_index].blocked_clients[j] = client_l[client_index].blocked_clients[j + 1];
			client_l[client_index].blocked_clients[j] = 0;
			client_l[client_index].num_blocked--;
			break;
		}
	}
}

int findClientfd(char *ip)
{
	for(int i = 0; i < num_clients; i++)
	{
		if(!strcmp(client_l[i].ip_addr, ip))
		{
			return client_l[i].sock_fd;
		}
	}
	return 0;
}

void getClientIPFromfd(int fd, char* IP)
{
	for(int i = 0; i < num_clients; i++)
	{
		if(client_l[i].sock_fd == fd)
		{
			strcpy(IP, client_l[i].ip_addr);
			break;
		}
	}
}
int getClientfdFromIndex(int index)
{
	return client_l[index].sock_fd;
}

int getClientIndexFromfd(int fd)
{
	for(int index = 0; index < num_clients; index++)
	{
		if(client_l[index].sock_fd == fd)
			return index;
	}
	return -1;
}

int getClientIndexFromIP(char* IP)
{
	int client_index = -1;
	if(IP)
	{
		for(int index = 0; index < num_clients; index++)
		{
			if(!strcmp(client_l[index].ip_addr, IP))
			{
				client_index = index;
				break;
			}
		}
	}
	return client_index;
}

void printList()
{
	struct hostent *he;
	struct sockaddr_in sa;
	int index = 1;
	for(int i = 0; i < num_clients; i++)
	{
		if(!isClientLoggedIn(i))
			continue;
		inet_pton(AF_INET, client_l[i].ip_addr, &(sa.sin_addr));
		he = gethostbyaddr(&sa.sin_addr.s_addr, sizeof(sa.sin_addr.s_addr), AF_INET);
		cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", index, he->h_name, client_l[i].ip_addr, client_l[i].port);
		index++;
	}
}

bool validateIP(char *IP)
{
	if(IP)
	{
		for(int i = 0; i < num_clients; i++)
		{
			if(!strcmp(client_l[i].ip_addr,IP))
				return true;
		}
	}
	return false;
}

void getList(struct client_details* client_list)
{
	memcpy(client_list, client_l, sizeof(client_l));
}

void parseClientList(char *data)
{
	char *tempPtr;
	int i = 0;
	tempPtr = strtok(data, "--,");
	while(tempPtr && (strcmp(tempPtr, " EndList")))
	{
		strcpy(client_l[i].ip_addr, tempPtr);
		tempPtr = strtok(NULL, "--,");
		client_l[i].port = atoi(tempPtr);
		if(tempPtr)
			tempPtr = strtok(NULL, "--,");
		client_l[i].bLoggedIn = true;
		i++;
	}
	num_clients = i;
}

void printStatistics()
{
	struct hostent *he;
	struct sockaddr_in sa;
	for(int i = 0; i < num_clients; i++)
	{
		inet_pton(AF_INET, client_l[i].ip_addr, &(sa.sin_addr));
		he = gethostbyaddr(&sa.sin_addr.s_addr, sizeof(sa.sin_addr.s_addr), AF_INET);
		cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8d\n", i + 1, he->h_name, client_l[i].num_msg_sent, client_l[i].num_msg_rcv, client_l[i].bLoggedIn);
	}
}
void printBlockedClientList(char* client_IP)
{
	int ind = -1;
	for(ind = 0; ind < num_clients; ind++)
	{
		if((client_IP) && client_l[ind].ip_addr && (!strcmp(client_l[ind].ip_addr, client_IP)))
		{
			break;
		}
	}
	if(ind >= 0)
	{
		int blocked_fd, blocked_index, list_ind = 0;
		struct client_details blocked_list[MAX_CLIENTS];
		for(int i = 0; i < client_l[ind].num_blocked; i++)
		{
			blocked_fd = client_l[ind].blocked_clients[i];
			blocked_index = getClientIndexFromfd(blocked_fd);
			if(blocked_index == -1)
				continue;
			if((list_ind == 0) || (blocked_list[list_ind - 1].port < client_l[blocked_index].port))
			{
				strcpy(blocked_list[list_ind].ip_addr, client_l[blocked_index].ip_addr);
				blocked_list[list_ind].port = client_l[blocked_index].port;
				blocked_list[list_ind].sock_fd = client_l[blocked_index].sock_fd;
				list_ind++;
			}
			else
			{
				for(int i = 0; i < list_ind; i++)
				{
					if(blocked_list[i].port < client_l[blocked_index].port)
						continue;
					else
					{
						for(int j = list_ind - 1; j >= i; j--)
						{
							strcpy(blocked_list[j + 1].ip_addr, blocked_list[j].ip_addr);
							blocked_list[j + 1].port = blocked_list[j].port;
							blocked_list[j + 1].sock_fd = blocked_list[j].sock_fd;
						}

						strcpy(blocked_list[i].ip_addr, client_l[blocked_index].ip_addr);
						blocked_list[i].port = client_l[blocked_index].port;
						blocked_list[i].sock_fd = client_l[blocked_index].sock_fd;
						list_ind++;
						break;
					}
				}
			}
		}
		struct hostent *he;
		struct sockaddr_in sa;
		for(int i = 0; i < client_l[ind].num_blocked; i++)
		{
			inet_pton(AF_INET, blocked_list[i].ip_addr, &(sa.sin_addr));
			he = gethostbyaddr(&sa.sin_addr.s_addr, sizeof(sa.sin_addr.s_addr), AF_INET);
			cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", i + 1, he->h_name, blocked_list[i].ip_addr, blocked_list[i].port);
		}
	}
}
char * tokenize(char * dest, char * src, char *delim)
{
	int pos = 0;
	char* temp = src;
	printf("src = %s\n", src);
	while(src && (*src != *delim || *(src + 1) != *(delim+1) || *(src + 2) != *(delim + 2)))
	{
		src++;
		pos ++;
	}
	printf("pos = %d\n", pos);
	if(src != NULL)
		strncpy(dest, temp, pos);
}
bool isBlocked(int client_index, int blocked_fd)
{
	for(int i = 0; i < client_l[client_index].num_blocked; i++)
	{
		if(client_l[client_index].blocked_clients[i] == blocked_fd)
			return true;
	}
	return false;
}
