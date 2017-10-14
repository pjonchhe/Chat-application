#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "../include/TCPserver.h"
#include "../include/common.h"
#include "../include/global.h"
#include "../include/logger.h"

int runServer()
{
	fd_set master_list, working_list;         //fd sets for select. working is a copy of master because a fresh copy needs to be sent to select everytime.
	int head_socket;                        //max socket number
	int rc;                                 //to store return value of select.
	int fd_ready;
	char *command_str;
        int server_socket;			//Master socket to handle incoming connections.
	struct sockaddr_in server_address;
	//struct sockaddr_in client_address[MAX_CLIENTS];
	int child_socket;
	char client_IP[HOSTNAME_LEN];
	char buffer[BUF_SIZE];
	char* data = NULL;
	int total = 0;
	int bytesLeft = BUF_SIZE;
        
	//Create master socket
	if(((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0))
	{
		printf("socket failed %d", errno);
		return 0;
	}

        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(getListenPort());
        server_address.sin_addr.s_addr = INADDR_ANY;

	//if(fcntl(server_socket, F_SETFL, O_NONBLOCK) < 0)
		//printf("error errno = %d\n", errno);
	//Bind socket to server address.
        if((bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address))) < 0)
	{
		printf("bind failed %d", errno);
		return 0;
	}

	//Listen on the socket
        if((listen(server_socket, 5)) < 0)
	{
		printf("listen failed %d", errno);
	}
	
	//Inititalize the master fd_set
	FD_ZERO(&master_list);
	FD_SET(STDIN, &master_list);
	FD_SET(server_socket, &master_list);
	head_socket = server_socket;

	while(true)
	{
		//Copy the master_list to working_list to send fresh copy to select everytime
		memcpy(&working_list, &master_list, sizeof(master_list));
		rc = select(head_socket + 1, &working_list, NULL, NULL, NULL);
		if(rc < 0)
		{
			printf("SELECT failed %d",errno);
			break;
		}
		fd_ready = rc;
		for (int actsock = 0; actsock <= head_socket && fd_ready > 0; ++actsock)
		{
			if (FD_ISSET(actsock, &working_list))
			{
				// ready descriptor found.
				fd_ready = -1;
				if(actsock == STDIN)
				{
					char buffer1[BUF_SIZE];
					memset(buffer1, 0, BUF_SIZE);
					read(STDIN, buffer1, BUF_SIZE);
					buffer1[strlen(buffer1)] = '\0';
					command_str = strtok(buffer1, " \r\n");
					if(command_str)
					{
						if(!(strcmp(command_str, "IP")))
						{
							cse4589_print_and_log("[IP:SUCCESS]\n");
							printIP();
							cse4589_print_and_log("[IP:END]\n");
						}
						else if(!(strcmp(command_str, "PORT")))
						{
							cse4589_print_and_log("[PORT:SUCCESS]\n");
							cse4589_print_and_log("PORT:%d\n", getListenPort());
							cse4589_print_and_log("[PORT:END]\n");
						}
						else if(!(strcmp(command_str, "AUTHOR")))
						{
							cse4589_print_and_log("[AUTHOR:SUCCESS]\n");
							printAuthor();
							cse4589_print_and_log("[AUTHOR:END]\n");
						}
						else if(!(strcmp(command_str, "LIST")))
						{
							cse4589_print_and_log("[LIST:SUCCESS]\n");
							printList();
							cse4589_print_and_log("[LIST:END]\n");
						}
						else if(!(strcmp(command_str, "STATISTICS")))
						{
							cse4589_print_and_log("[STATISTICS:SUCCESS]\n");
							printStatistics();
							cse4589_print_and_log("[STATISTICS:END]\n");
						}
						else if(!(strcmp(command_str, "BLOCKED")))
						{
							command_str = strtok(NULL, " \r\n");
							bool bError = true;
							if(command_str)
							{
								bool valid = validateIP(command_str);
								if(valid)
								{
									cse4589_print_and_log("[BLOCKED:SUCCESS]\n");
									printBlockedClientList(command_str);
									cse4589_print_and_log("[BLOCKED:END]\n");
									bError = false;
								}
							}
							if(bError)
							{
								cse4589_print_and_log("[BLOCKED:ERROR]\n");
								cse4589_print_and_log("[BLOCKED:END]\n");
							}
						}
						else
						{
							cse4589_print_and_log("[%s:ERROR]\n", command_str);
							cse4589_print_and_log("[%s:END]\n", command_str);
						}
					}
				}
				else if(actsock == server_socket)
				{
					struct sockaddr_in client_address;
					socklen_t len = sizeof(client_address);
					// accept the incoming connection.
					if((child_socket = accept(server_socket, (struct sockaddr *)&client_address, &len)) < 0)
					{
						printf("accept failed %d", errno);
						break;
					}
					//if(fcntl(child_socket, F_SETFL, O_NONBLOCK) < 0)
						//printf("ERROR errno = %d\n", errno);
					inet_ntop(AF_INET, &(client_address.sin_addr), client_IP, sizeof(client_IP));
					//printf("CLIENT_IP = %s\n", client_IP);
					//addClient(client_address, child_socket);
					
					/*struct client_details client_list[MAX_CLIENTS];
					getList(client_list);
					char server_message[BUF_SIZE] = "LoggedIn ";
					for(int j = 0; j < getNumClients(); j++)
					{
						if(!isClientLoggedIn(j))
							continue;
						char tempBuf[10];
						sprintf(tempBuf, "%d", client_list[j].port);
						strcat(server_message, client_list[j].ip_addr);
						strcat(server_message, "--");
						strcat(server_message, tempBuf);
						strcat(server_message, ",");
					}
					strcat(server_message, " EndList");
					//Send list of logged in clients in the format "LoggedIn IP1--Port1,IP2--Port2 EndList"
					send(child_socket, server_message, strlen(server_message), 0);
					*/
					//Add child socket fd to master list.
					FD_SET(child_socket, &master_list);

					//Increment the head socket if child socket is greater than head socket
					if(head_socket < child_socket)
					{
						head_socket = child_socket;
					}
				}
				else
				{
					bool bBreakLoop = false;
					int len;
					char senderIP[HOSTNAME_LEN];
					char *receiver_IP;
					int receiver_fd;
					char *recv_msg;
					char send_msg[BUF_SIZE];
					int sender_index;
					int receiver_index;
					//char buffer[BUF_SIZE];
					char tempBufMain[BUF_SIZE];
					//char* data = NULL;
					//int total = 0;
					//int bytesLeft = BUF_SIZE;
					//while(true)
					{
						memset(buffer, 0, BUF_SIZE);
						/*if(data)
						{
							if(data[strlen(data) - 1] == '$' && data[strlen(data) - 2] == '&')
							{
								if(data[0] == '&' && data[1] == '$')
								{
									bytesLeft = BUF_SIZE;
									total = 0;
								}
								else
								{
									strncpy(buffer, data, strlen(data) - 2);
									bytesLeft = BUF_SIZE - strlen(data);
									total = strlen(data) - 2;
									//memset(data, 0, BUF_SIZE);
								}
							}
						}*/
						int lenCount = 3;
						char msgLen[3];
						len = recv(actsock, msgLen, 3, 0);
						int total1 = 0;
						int lenMsg = atoi(msgLen);
						int bytesleft1 = lenMsg;
						int n;
						while(total1 < lenMsg)
						{
							n = recv(actsock, buffer + total1, bytesleft1, 0);
							total1 += n;
							bytesleft1 -= n;
							if(total1 == lenMsg)
								break;
							/*if(0 == n)
							{
								close(actsock);
								FD_CLR(actsock, &master_list);
								break;
							}
							if(-1 == len)
							{
								if(errno == EAGAIN || errno == EWOULDBLOCK)
									break;
							}*/
						}
						
						
                                                //len = recv(actsock, buffer + total, bytesLeft - 2, 0);
						printf("RECEIVED buffer len = %d buffer = %s\n", len, buffer);
						//buffer[strlen(buffer)] = '&';
						//buffer[strlen(buffer) + 1] = '$';
						/*if(!strstr(buffer, "REFRESH"))
						{
							strcat(buffer, "&$");
							printf("Modified buffer = %s\n", buffer);
							fflush(stdout);
							if(0 == len)
							{
								close(actsock);
								FD_CLR(actsock, &master_list);
								break;
							}
							if(-1 == len)
							{
								if(errno == EAGAIN || errno == EWOULDBLOCK)
									break;
							}
							/*else if(strstr(buffer, ""))
							{
								bBreakLoop = true;
							}*/
						/*	strcpy(tempBufMain, buffer);
							//data = strtok(buffer, "|");
							//tokenize(data, tempBufMain, "|");
							data = strtok(tempBufMain, "|");
							printf("data = %s\n", data);
							fflush(stdout);
						}
						else*/ if(strstr(buffer, "REFRESH"))
						{
							char server_message[BUF_SIZE];
							memset(server_message, 0, BUF_SIZE);
							char temp_msg[BUF_SIZE];
							memset(temp_msg, 0, BUF_SIZE);
							strcat(temp_msg, "REFRESH ");
							printf("received refresh\n");
							struct client_details client_list[MAX_CLIENTS];
							getList(client_list);
							for(int j = 0; j < getNumClients(); j++)
							{
								if(isClientLoggedIn(j))
								{
									char tempBuf[10];
									sprintf(tempBuf, "%d", client_list[j].port);
									strcat(temp_msg, client_list[j].ip_addr);
									strcat(temp_msg, "--");
									strcat(temp_msg, tempBuf);
									strcat(temp_msg, ",");
								}
							}
							strcat(temp_msg, " EndList");
							int msgLen = strlen(temp_msg);
							char tempLen[3];
							memset(tempLen, 0, 3);
							if(msgLen < 10)
								strcat(server_message, "00");
							else if(msgLen < 100)
								strcat(server_message, "0");
							sprintf(tempLen, "%d", msgLen);
							strcat(server_message, tempLen);
							strcat(server_message, temp_msg);
							send(actsock, server_message, strlen(server_message), 0);
						}
					
						//while(data && data[strlen(data) - 1] != '$' && data[strlen(data) - 2] != '&')
						{
							receiver_IP = strtok(buffer, " ");
							//int count = 0;
							//char *temp = data;
							//printf("temp = %s\n", temp);
							//char receiver_IP[BUF_SIZE];
							//memset(receiver_IP, 0, BUF_SIZE);
							/*while(temp && *temp != ' ')
							{
								temp++;
								count++;
								printf("temp = %s, count = %d\n", temp, count);
							}*/
							
							//strncpy(receiver_IP, data, count);
							printf("receiver_IP = %s\n", receiver_IP);
							//strcpy(recv_msg, temp);
							//recv_msg = temp;
							//recv_msg = strtok(buffer, " ");
							//printf("recv_msg = %s\n", recv_msg);
							fflush(stdout);
							if(receiver_IP && strstr(receiver_IP, "BROADCAST"))
							{
								recv_msg = strtok(NULL, "\r\n");
								getClientIPFromfd(actsock, senderIP);
								sender_index = getClientIndexFromfd(actsock);
								incrementSentMsgCnt(sender_index);
								char* temp_msg = (char*)malloc(BUF_SIZE);
								memset(temp_msg, 0, BUF_SIZE);
								memset(send_msg, 0, BUF_SIZE);
								strcpy(temp_msg, senderIP);
								strcat(temp_msg, " ");
								strcat(temp_msg, recv_msg);
								cse4589_print_and_log("[RELAYED:SUCCESS]\n");
								cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", senderIP, BROADCAST_IP, recv_msg);
								//strcat(temp_msg, "|");
								cse4589_print_and_log("[RELAYED:END]\n");
								int msgLen = strlen(temp_msg);
								char tempLen[3];
								memset(tempLen, 0, 3);
								if(msgLen < 10)
									strcat(send_msg, "00");
								else if(msgLen < 100)
									strcat(send_msg, "0");
								sprintf(tempLen, "%d", msgLen);
								strcat(send_msg, tempLen);
								strcat(send_msg, temp_msg);
							
								for(int indx = 0; indx < getNumClients(); indx++)
								{
									receiver_fd = getClientfdFromIndex(indx);
									if((actsock != receiver_fd) && (!isBlocked(indx, actsock)) && (isClientLoggedIn(indx)))
									{
										send(receiver_fd, send_msg, strlen(send_msg), 0);
										incrementRecvdMsgCnt(indx);
									}
									else if((actsock != receiver_fd) && (!isBlocked(indx, actsock)) && (!isClientLoggedIn(indx)))
									{
										char f_name[10];
										memset(f_name, 0, 10);
										sprintf(f_name, "%d", receiver_fd);
										strcat(f_name, ".txt");
										FILE *buff_fd = fopen(f_name, "ab");
										if(buff_fd)
										{
											printf("FILE OPENED\n");
											strcat(send_msg, "\n");
											fwrite (send_msg , sizeof(char), strlen(send_msg), buff_fd);
											fclose(buff_fd);
										}
									}
								}
								free(temp_msg);
							}
							else if(receiver_IP && strstr(receiver_IP, "BLOCK"))
							{
								char* block_IP = strtok(NULL, "\r\n");
								//printf("block_ip = %s\n", block_IP);
								//char block_IP[HOSTNAME_LEN];
								//memset(block_IP, 0, HOSTNAME_LEN);
								//strncpy(block_IP, recv_msg + 1, strlen(recv_msg +1));
								//char* block_IP = recv_msg;
								fflush(stdout);
								int block_fd = findClientfd(block_IP);
								//printf("block_fd = %d\n",block_fd);
								sender_index = getClientIndexFromfd(actsock);
								//printf("index - %d\n",sender_index);
								fflush(stdout);
								blockClientWithfd(block_fd, sender_index);
							}
							else if(receiver_IP && strstr(receiver_IP, "UNBLK"))
							{
								char* unblock_IP = strtok(NULL, "\r\n");
								//char unblock_IP[HOSTNAME_LEN];
								//memset(unblock_IP, 0, HOSTNAME_LEN);
								//strncpy(unblock_IP, recv_msg + 1, strlen(recv_msg +1));
								int unblock_fd = findClientfd(unblock_IP);
								sender_index = getClientIndexFromfd(actsock);
								printf("RECEIVED UNBLOCK\n");
								unblockClientWithfd(unblock_fd, sender_index);
							}
							else if(receiver_IP && strstr(receiver_IP, "LOGOUT"))
							{
								sender_index = getClientIndexFromfd(actsock);
								setClientLoggedIn(sender_index, false);
							}
							else if(receiver_IP && strstr(receiver_IP, "ACTIVATE"))
							{
								sender_index = getClientIndexFromfd(actsock);
								setClientLoggedIn(sender_index, true);
								struct client_details client_list[MAX_CLIENTS];
								getList(client_list);
								char server_message[BUF_SIZE];
								memset(server_message, 0, BUF_SIZE);
								char temp_msg[BUF_SIZE];
								memset(temp_msg, 0, BUF_SIZE);
								strcat(temp_msg, "LoggedIn ");
								char f_name[10];
								memset(f_name, 0, 10);
								sprintf(f_name, "%d", actsock);
								strcat(f_name, ".txt");
								FILE *buff_fd = fopen(f_name, "rb");
								/*if(!buff_fd)
								{
									strcpy(server_message, "LoggedIn ");
								}
								else
								{
									strcpy(server_message, "Buffered ");
								}*/
								for(int j = 0; j < getNumClients(); j++)
								{
									if(!isClientLoggedIn(j))
										continue;
									char tempBuf[10];
									sprintf(tempBuf, "%d", client_list[j].port);
									strcat(temp_msg, client_list[j].ip_addr);
									strcat(temp_msg, "--");
									strcat(temp_msg, tempBuf);
									strcat(temp_msg, ",");
								}
								strcat(temp_msg, " EndList");
								int msgLen = strlen(temp_msg);
								char tempLen[3];
								memset(tempLen, 0, 3);
								if(msgLen < 10)
									strcat(server_message, "00");
								else if(msgLen < 100)
									strcat(server_message, "0");
								sprintf(tempLen, "%d", msgLen);
								strcat(server_message, tempLen);
								strcat(server_message, temp_msg);
								//Send list of logged in clients in the format "LoggedIn IP1--Port1,IP2--Port2 EndList"
								send(actsock, server_message, strlen(server_message), 0);
								if(buff_fd)
								{
									char* send_msg2 = (char*)malloc(BUF_SIZE);
									char c[1];
									int ch;
									/*do
									{
										memset(send_msg, 0, BUF_SIZE);
										while(true)
										{
											ch = fgetc(buff_fd);
											strcat(send_msg, (char*)ch);
											if(strstr(send_msg, "|"))
												break;
										}
										printf("send_msg is %s", send_msg);
									}while(ch != EOF);*/
									while(fgets (send_msg2 , BUF_SIZE , buff_fd) != NULL)
									{
										//printf("send_msg is %s", send_msg);
										//sleep(1);
										incrementRecvdMsgCnt(sender_index);
										//rcat(send_msg2, " ");
										int total1 = 0;
										int bytesleft1 = strlen(send_msg2);
										int n;
										
										while(total1 < strlen(send_msg2))
										{
											n = send(actsock, send_msg2 + total1, bytesleft1, 0);
											if(n == -1)
											{
												break;
											}
											total1 += n;
											bytesleft1 -= n;
										}
										
										printf("send = %s\n", send_msg2);
										//printf("ret = %d\n", ret);
									}
									fclose(buff_fd);
									remove(f_name);
									free(send_msg2);
									//send(i, "Endofmsg", 9, 0);
								}
								int total2 = 0;
								//char tempeom[8] = "Endofmsg";
								//char eom[20];
								/*memset(eom, 0, 20);
								int msgLen = strlen(tempeom);
								char tempLen[3];
								memset(tempLen, 0, 3);
								if(msgLen < 10)
									strcat(eom, "00");
								else if(msgLen < 100)
									strcat(eom, "0");
								sprintf(tempLen, "%d", msgLen);
								strcat(eom, tempLen);
								strcat(eom, temp_eom);
				
								send(actsock, eom, strlen(eom), 0);*/
								send(actsock, "008Endofmsg", 11, 0);
							}
							else if(receiver_IP && strstr(receiver_IP, "EXIT"))
							{
								removeClient(actsock);
								clearFromBlockedList(actsock);
								close(actsock);
								FD_CLR(actsock, &master_list);
								char f_name[10];
								sprintf(f_name, "%d", actsock);
								strcat(f_name, ".txt");
								int ret = remove(f_name);
								if(ret == 0)
									printf("FILE DELETED\n");
								else
									printf("FAILED\n");
								break;
							}
							else if(receiver_IP && strstr(receiver_IP, "PORT"))
							{
								char* port = strtok(NULL, "\r\n");
								//char *port = temp;
								int client_port = atoi(port);
								//printf("GOT PORT\n");
								//int client_index = getClientIndexFromfd(i);
								//setClientPort(client_port, client_index);
								addClient(client_IP, actsock, client_port);
								printf("GOT port:%d from clientfd = %d index = %d\n",client_port, actsock, getClientIndexFromfd(actsock));
								fflush(stdout);
								struct client_details client_list[MAX_CLIENTS];
								getList(client_list);
								char server_message[BUF_SIZE];
								memset(server_message, 0, BUF_SIZE);
								char temp_msg[BUF_SIZE];
								memset(temp_msg, 0, BUF_SIZE);
								strcat(temp_msg, "LoggedIn ");
								for(int j = 0; j < getNumClients(); j++)
								{
									if(!isClientLoggedIn(j))
										continue;
									char tempBuf[10];
									memset(tempBuf, 0, 10);
									sprintf(tempBuf, "%d", client_list[j].port);
									strcat(temp_msg, client_list[j].ip_addr);
									strcat(temp_msg, "--");
									strcat(temp_msg, tempBuf);
									strcat(temp_msg, ",");
								}
								strcat(temp_msg, " EndList");
								//strcat(temp_msg, "Endofmsg");
								//printf("SEND PORT\n");
								fflush(stdout);
								int msgLen = strlen(temp_msg);
								char tempLen[3];
								memset(tempLen, 0, 3);
								if(msgLen < 10)
									strcat(server_message, "00");
								else if(msgLen < 100)
									strcat(server_message, "0");
								sprintf(tempLen, "%d", msgLen);
								strcat(server_message, tempLen);
								strcat(server_message, temp_msg);
								//Send list of logged in clients in the format "LoggedIn IP1--Port1,IP2--Port2 EndList"
								send(actsock, server_message, strlen(server_message), 0);
								send(actsock, "008Endofmsg", 11, 0);
							}
							else if(receiver_IP)
							{
								int ki = 0;
								//char *temp = strtok(NULL, "\r\n");
								//recv_msg = strtok(temp, "");
								recv_msg = strtok(NULL, "\r\n");
								//while(recv_msg)
								{
									receiver_fd =  findClientfd(receiver_IP);
									receiver_index = getClientIndexFromfd(receiver_fd);
									printf("RECEIVER INDEX = %d\ receiver IP = %s receiver_fd = %d\n", receiver_index, receiver_IP, receiver_fd);
									fflush(stdout);
									if(isBlocked(receiver_index, actsock))
									{
										printf("BLOCKED !!!!\n");
										recv_msg = strtok(NULL, "");
										//receiver_IP = strtok(buffer, " ");
										//continue;
									}
		
									//recv_msg = strtok(NULL, "");
									//printf("recv_msg = %s\n", recv_msg);
									if(recv_msg && !(isBlocked(receiver_index, actsock)))
									{
										getClientIPFromfd(actsock, senderIP);
										//printf("senderfd = %d senderIP = %s\n",i, senderIP);
										sender_index = getClientIndexFromfd(actsock);
										incrementSentMsgCnt(sender_index);
										char* temp_msg = (char*)malloc(BUF_SIZE);
										memset(temp_msg, 0, BUF_SIZE);
										memset(send_msg, 0, BUF_SIZE);
										//printf("send_msg = %s\n", send_msg);
										//printf("recv_msg = %s\n", recv_msg);
										strcpy(temp_msg, senderIP);
										strcat(temp_msg, " ");
										strcat(temp_msg, recv_msg);
										//strcat(temp_msg, "|");
										cse4589_print_and_log("[RELAYED:SUCCESS]\n");
										cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", senderIP, receiver_IP, recv_msg);
										cse4589_print_and_log("[RELAYED:END]\n");
										printf("receiver_fd = %d receiver_index = %d, actsock = %d sender_index = %d\n", receiver_fd, receiver_index, actsock, sender_index);
										int msgLen = strlen(temp_msg);
										char tempLen[3];
										memset(tempLen, 0, 3);
										if(msgLen < 10)
											strcat(send_msg, "00");
										else if(msgLen < 100)
											strcat(send_msg, "0");
										sprintf(tempLen, "%d", msgLen);
										strcat(send_msg, tempLen);
										strcat(send_msg, temp_msg);
										
										if((receiver_fd != 0) && (isClientLoggedIn(receiver_index)))
										{
											/*cse4589_print_and_log("[RELAYED:SUCCESS]\n");
											cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", senderIP, receiver_IP, recv_msg);
											cse4589_print_and_log("[RELAYED:END]\n");*/
											//printf("final send = %s\n",send_msg);
											int total1= 0;
											int bytesleft1 = strlen(send_msg);
											int n;
											while(total1 < strlen(send_msg))
											{
												n = send(receiver_fd, send_msg + total1, bytesleft1, 0);
												if(n == -1)
													break;
												total1 += n;
												bytesleft1 -= n;
												printf("SEND bytes = %d", n);
											}
											incrementRecvdMsgCnt(receiver_index);
										}
										else if(receiver_fd > 2 && !(isClientLoggedIn(receiver_index)))
										{
											char f_name[10];
											sprintf(f_name, "%d", receiver_fd); 
											strcat(f_name, ".txt");
											FILE *buff_fd = fopen(f_name, "ab");
											if(buff_fd)
											{
												//printf("FILE OPENED\n");
												//strcat(send_msg, "\n");
												fwrite (send_msg , sizeof(char), strlen(send_msg), buff_fd);
												fclose(buff_fd);
											}
										}
										//receiver_IP = strtok(NULL, " ");
										//printf("receiver_IP = %s\n",receiver_IP);
										//receiver_IP = receiver_IP + 2;
										//printf("receiver_IP = %s\n",receiver_IP);
										if(receiver_IP)
										{
											//recv_msg = strtok(NULL, "|");
										}
										free(temp_msg);
fflush(stdout);	
									}
								}
							}

							/*data = strtok(NULL, "|");
							if(data && data[strlen(data) - 1] == '$' && data[strlen(data) - 2] == '&')
							{
								//memset(buffer, 0, BUF_SIZE);
								/*if(data[0] == '&' && data[1] == '$')
								{
									bytesLeft = BUF_SIZE;
									total = 0;
								}
								else
								{
									strncpy(buffer, data, strlen(data) - 2);
									bytesLeft = BUF_SIZE - strlen(data);
									total = strlen(data) - 2;
									memset(data, 0, BUF_SIZE);
								}*/
								/*break;
							}*/
						
							if(bBreakLoop)
								break;
						}
					}
				}
			}
		}
	}
        close(server_socket);
        return 0;
}

