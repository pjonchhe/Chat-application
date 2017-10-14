#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "../include/TCPclient.h"
#include "../include/common.h"
#include "../include/global.h"
#include "../include/logger.h"

using namespace std;

bool bLoggedIn = false;
bool bConnServer = false;
int block_index[MAX_CLIENTS];
int blocked_num = 0;

int startClient(char* serverIP)
{
	fd_set master_list, working_list;		//fd sets for select. working is a copy of master because a fresh copy needs to be sent to select everytime.
	int sock = -1;
	int head_socket;			//max socket number
	int rc;					//to store return value of select.
	int fd_ready;
	char *command_str;
	int socket;
	char buffer[BUF_SIZE];
	char* data = NULL;
	int total = 0;
	int bytesLeft = BUF_SIZE;

	//network_socket = socket(AF_INET, SOCK_STREAM, 0);

	//specify an address for the socket
	/*struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	inet_pton(AF_INET, serverIP, &server_address.sin_addr.s_addr);*/

	//Inititalize the master fd_set
	int p2p_skt = runServerP2P();
	FD_ZERO(&master_list);
	//head_socket = STDIN;
	FD_SET(STDIN, &master_list);
	FD_SET(p2p_skt, &master_list);
	head_socket = p2p_skt;

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
		for (int i=0; i <= head_socket && fd_ready > 0; ++i)
		{
			if (FD_ISSET(i, &working_list))
			{
				// ready descriptor found.
				fd_ready = -1;
				if(i == STDIN)
				{
					char buffer2[300];
					memset(buffer2, 0, 300);
					read(STDIN, buffer2, 300);
					buffer2[strlen(buffer2)] = '\0';
					command_str = strtok(buffer2, " \r\n");
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
						else if(!bLoggedIn && !(strcmp(command_str, "LOGIN")))
						{
							char server_IP[HOSTNAME_LEN];
							int server_port;
							int send1;
							bool bLocalError = true;
							send1 = 0;
							printf("GOT login command\n");
							fflush(stdout);
							command_str = strtok(NULL, " \r\n");
							if(command_str)
							{
								strcpy(server_IP, command_str);
								command_str = strtok(NULL, " \r\n");
								if(command_str)
								{
									server_port = atoi(command_str);
									printf("server_port is %d\n", server_port);
									fflush(stdout);
									if(!bConnServer)
									{
										printf("START CLIENT\n");
										fflush(stdout);
										socket = runClient(server_IP, server_port);
										if(0 != socket)
											bConnServer = true;
									}
									else
									{
										char *login_msg = (char*)malloc(BUF_SIZE/8);
										char *temp_msg = (char*) malloc(BUF_SIZE/8);
										memset(temp_msg, 0, BUF_SIZE/8);
										memset(login_msg, 0, BUF_SIZE/8);
										strcat(temp_msg, "ACTIVATE");
										int msgLen = strlen(temp_msg);
										char tempLen[3];
										memset(tempLen, 0, 3);
										if(msgLen < 10)
										{
											strcat(login_msg, "00");
										}
										else if(msgLen < 100)
										{
											strcat(temp_msg, "0");
										}
										sprintf(tempLen, "%d", msgLen);
										strcat(login_msg, tempLen);
										strcat(login_msg, temp_msg);
										int total1 = 0;
										int bytesleft1 = strlen(login_msg);
										while(total1 < strlen(login_msg))
										{
											send1 = send(socket, login_msg + total1, bytesleft1, 0);
											if(send1 == -1)
												break;
											total1 += send1;
											bytesleft1 -= send1;
										}
										
										free(temp_msg);
										temp_msg = NULL;
										free(login_msg);
										login_msg = NULL;
									}
									if(0 != socket && (send1 != -1))
									{
										bLocalError = false;
										FD_SET(socket, &master_list);
										if(socket > head_socket)
										{
											head_socket = socket;
										}
									}
								}
								if(bLocalError)
								{
									cse4589_print_and_log("[LOGIN:ERROR]\n");
									cse4589_print_and_log("[LOGIN:END]\n");
								}
							}
						}
						else if(bLoggedIn && !(strcmp(command_str, "LIST")))
                                                {
							cse4589_print_and_log("[LIST:SUCCESS]\n");
                                                        printList();
							cse4589_print_and_log("[LIST:END]\n");
                                                }
						else if(bLoggedIn && !(strcmp(command_str, "SEND")))
						{
							int send1;
							bool bLocalError = false;
							char *send_msg = (char*)malloc(BUF_SIZE);
							char *temp_send = (char*)malloc(BUF_SIZE);
							memset(send_msg, 0, BUF_SIZE);
							memset(temp_send, 0, BUF_SIZE);
							char IP[20];
							send1 = 0;
							command_str = strtok(NULL, " ");
							strcpy(IP, command_str);
							bool valid = validateIP(IP);
							if(valid)
							{
								command_str = strtok(NULL, "\r\n");
								//printf("command_str = %s\n", command_str);
								char *test = "hello this is prajin $ $ @#^&*() this is after this is after hi thi is athe ad ad adf the asdfasdf asdfasdf lkmt htis is tis theod aitd todng tjlenf sdfjalsd tere adkfa the sdaf adf rard fafas afkrgja this is the end this is the end this is the end this is the end this is the end this is the end this is the end this is the end this is the end this is the end";
								
								strcat(temp_send, IP);
								strcat(temp_send, " ");
								strcat(temp_send, command_str);
								//strcat(temp_send, "|");
								int loop = 0;
								int msgLen = strlen(temp_send);
								char tempLen[3];
								memset(tempLen, 0, 3);
								if(msgLen < 10)
								{
									strcat(send_msg, "00");
								}
								else if(msgLen < 100)
								{
									strcat(send_msg, "0");
								}
								sprintf(tempLen, "%d", msgLen);
								strcat(send_msg, tempLen);
								strcat(send_msg, temp_send);
								//while(loop < 1000)
								{
								int total1 = 0;
								int bytesleft1 = strlen(send_msg);
								while(total1 < strlen(send_msg))
								{
									send1 = send(socket, send_msg + total1, bytesleft1, 0);
									if(send1 == -1)
										break;
									total1 += send1;
									bytesleft1 -= send1;
									printf("Send bytes: %d strlen(send_msg) = %d\n", send1, strlen(send_msg));
								}
								loop++;
								}
							}
							else
							{
								bLocalError = true;
							}
							if(send1 != -1 && !bLocalError)
							{
								cse4589_print_and_log("[SEND:SUCCESS]\n");
								cse4589_print_and_log("[SEND:END]\n");
							}
							else
							{
								cse4589_print_and_log("[SEND:ERROR]\n");
								cse4589_print_and_log("[SEND:END]\n");
							}
							free(temp_send);
							free(send_msg);
						}
						else if(bLoggedIn && !(strcmp(command_str, "BROADCAST")))
						{
							char *broadcast_msg = (char*)malloc(BUF_SIZE);
							char *temp_msg = (char*)malloc(BUF_SIZE);
							memset(temp_msg, 0, BUF_SIZE);
							memset(broadcast_msg, 0, BUF_SIZE);
							strcat(temp_msg, "BROADCAST ");
							command_str = strtok(NULL, "\r\n");
							if(command_str)
							{
								strcat(temp_msg, command_str);
								//strcat(temp_msg, "|");
								int msgLen = strlen(temp_msg);
								char tempLen[3];
								memset(tempLen, 0, 3);
								if(msgLen < 10)
									strcat(broadcast_msg, "00");
								else if(msgLen < 100)
									strcat(broadcast_msg, "0");
								sprintf(tempLen, "%d", msgLen);
								strcat(broadcast_msg, tempLen);
								strcat(broadcast_msg, temp_msg);
								send(socket, broadcast_msg, strlen(broadcast_msg), 0);
								cse4589_print_and_log("[BROADCAST:SUCCESS]\n");
								cse4589_print_and_log("[BROADCAST:END]\n");
							}
							else
							{
								cse4589_print_and_log("[BROADCAST:ERROR]\n");
								cse4589_print_and_log("[BROADCAST:END]\n");
							}
							free(broadcast_msg);
						}
						else if(bLoggedIn && !(strcmp(command_str, "REFRESH")))
						{
							int send1;
							send1 = 0;
							send1 = send(socket, "007REFRESH", 10, 0);
							if(send1 == -1)
							{
								cse4589_print_and_log("[REFRESH:ERROR]\n");
								cse4589_print_and_log("[REFRESH:END]\n");
							}
							
						}
						else if(bLoggedIn && !(strcmp(command_str, "BLOCK")))
						{
							char *block_msg = (char*)malloc(BUF_SIZE/4);
							char *temp_msg = (char*)malloc(BUF_SIZE/4);
							memset(temp_msg, 0, BUF_SIZE/4);
							memset(block_msg, 0, BUF_SIZE/4);
							bool bError = true;
							bool blocked = false;
							int indx;
							strcat(temp_msg, "BLOCK ");
							command_str = strtok(NULL, "\r\n");
							if(command_str)
							{
								bool valid = validateIP(command_str);
								int k;
								indx = getClientIndexFromIP(command_str);
								for(k = 0; k < blocked_num; k++)
								{
									if(block_index[k] == indx)
									{
										blocked = true;
										break;
									}
								}
								if(valid && !blocked && indx != -1)
								{
									int send1;
									send1 = 0;
									strcat(temp_msg, command_str);
									//strcat(block_msg, "|");
									int msgLen = strlen(temp_msg);
									char tempLen[3];
									memset(tempLen, 0, 3);
									if(msgLen < 10)
										strcat(block_msg, "00");
									else if(msgLen < 100)
										strcat(block_msg, "0");
									sprintf(tempLen, "%d", msgLen);
									strcat(block_msg, tempLen);
									strcat(block_msg, temp_msg);
									send1 = send(socket, block_msg, strlen(block_msg), 0);
									if(send1 != -1)
									{
										block_index[blocked_num] = indx;
										blocked_num++;
										cse4589_print_and_log("[BLOCK:SUCCESS]\n");
										cse4589_print_and_log("[BLOCK:END]\n");
										bError = false;
									}
								}
							}
							if(bError)
							{
								cse4589_print_and_log("[BLOCK:ERROR]\n");
								cse4589_print_and_log("[BLOCK:END]\n");
							}
							free(block_msg);
							free(temp_msg);
						}
						else if(bLoggedIn && !(strcmp(command_str, "UNBLOCK")))
						{
							char *unblock_msg = (char*)malloc(BUF_SIZE/4);
							char *temp_msg = (char*)malloc(BUF_SIZE/4);
							memset(unblock_msg, 0, BUF_SIZE/4);
							memset(temp_msg, 0, BUF_SIZE/4);
							int indx;
							bool bError = true;
							bool blocked = false;
							strcat(temp_msg, "UNBLK ");
							command_str = strtok(NULL, "\r\n");
							if(command_str)
							{
								bool valid = validateIP(command_str);
								int k;
								indx = getClientIndexFromIP(command_str);
								for(k = 0; k < blocked_num; k++)
								{
									if(block_index[k] == indx)
									{
										blocked = true;
										break;
									}
								}
								if(valid && blocked && indx != -1)
								{
									int send1;
									send1 = 0;
									strcat(temp_msg, command_str);
									//strcat(temp_msg, "|");
									int msgLen = strlen(temp_msg);
									char tempLen[3];
									memset(tempLen, 0, 3);
									if(msgLen < 10)
										strcat(unblock_msg, "00");
									else if(msgLen < 100)
										strcat(unblock_msg, "0");
									sprintf(tempLen, "%d", msgLen);
									strcat(unblock_msg, tempLen);
									strcat(unblock_msg, temp_msg);	
									send1 = send(socket, unblock_msg, strlen(unblock_msg), 0);
									if(send1 != -1)
									{
										for(int m = k; m < blocked_num - 1; m++)
										{
											block_index[m] = block_index[m + 1];
										}
										block_index[blocked_num - 1] = 0;
										blocked_num--;
										cse4589_print_and_log("[UNBLOCK:SUCCESS]\n");
										cse4589_print_and_log("[UNBLOCK:END]\n");
										bError = false;
									}
								}
							}
							if(bError)
							{
								cse4589_print_and_log("[UNBLOCK:ERROR]\n");
								cse4589_print_and_log("[UNBLOCK:END]\n");
							}
							free(unblock_msg);
							free(temp_msg);
						}
						else if(bLoggedIn && !(strcmp(command_str, "LOGOUT")))
						{
							/*char *logout_msg = (char*)malloc(BUF_SIZE/2);
							char *temp_msg = (char*)malloc(BUF_SIZE/2);
							memset(logout_msg, 0, BUF_SIZE/2);
							memset(temp_msg, 0, BUF_SIZE/2);
							int send1;
							send1 = 0;
							strcat(temp_msg, "LOGOUT");
							int msgLen = strlen(temp_msg);
							char tempLen[3];
							memset(tempLen, 0, 3);
							if(msgLen < 10)
								strcat(logout_msg, "00");
							else if(msgLen < 100)
								strcat(logout_msg, "0");
							sprintf(tempLen, "%d", msgLen);
							strcat(logout_msg, tempLen);
							strcat(logout_msg, temp_msg);*/
							//send1 = send(socket, logout_msg, strlen(logout_msg), 0);
							int send1;
							send1 = send(socket, "006LOGOUT", 9, 0);
							if(send1 != -1)
							{
								cse4589_print_and_log("[LOGOUT:SUCCESS]\n");
								cse4589_print_and_log("[LOGOUT:END]\n");
							}
							else
							{
								cse4589_print_and_log("[LOGOUT:ERROR]\n");
								cse4589_print_and_log("[LOGOUT:END]\n");
							}
							//free(temp_msg);
							//free(logout_msg);
							bLoggedIn = false;
						}
						else if(!(strcmp(command_str, "EXIT")))
						{
							char *exit_msg = (char*)malloc(BUF_SIZE/8);
							char *temp_msg = (char*)malloc(BUF_SIZE/8);
							memset(temp_msg, 0, BUF_SIZE/8);
							memset(exit_msg, 0, BUF_SIZE/8);
							strcat(temp_msg, "EXIT");
							int msgLen = strlen(temp_msg);
							char tempLen[3];
							memset(tempLen, 0, 3);
							if(msgLen < 10)
								strcat(exit_msg, "00");
							else if(msgLen < 100)
								strcat(exit_msg, "0");
							sprintf(tempLen, "%d", msgLen);
							strcat(exit_msg, tempLen);
							strcat(exit_msg, temp_msg);
							send(socket, exit_msg, strlen(exit_msg) + 1, 0);
							close(socket);
							free(exit_msg);
							free(temp_msg);
							cse4589_print_and_log("[EXIT:SUCCESS]\n");
							cse4589_print_and_log("[EXIT:END]\n");
							exit(0);
						}
						else if(bLoggedIn && !strcmp(command_str, "SENDFILE"))
						{
							char* client_IP = strtok(NULL, " ");
							char* tempName = strtok(NULL, "");
							int socket;
							printf("client_IP = %s\n", client_IP);
							//printf("filenma = %s\n", fileName);
							char fileName[25];
							memset(fileName, 0, 25);
							int t = strlen(tempName);
							strncpy(fileName, tempName, strlen(tempName) - 1);
							if(client_IP)
							{
								bool valid = validateIP(client_IP);
								if(valid)
								{
									FILE* pFile;
									pFile = fopen(fileName, "rb");
									if(!pFile)
									{
										cse4589_print_and_log("[SENDFILE:ERROR]\n");
										cse4589_print_and_log("[SENDFILE:END]\n");
									}
									else
									{
										int indx = getClientIndexFromIP(client_IP);
										int remotePort = getClientPort(indx);
										socket = runClientP2P(client_IP, remotePort);
										if(socket)
										{
											printf("Connection successful\n");
											char* p2p_Buf = (char*)malloc(BUF_SIZE);
											memset(p2p_Buf, 0, BUF_SIZE);
											int lenFileName = strlen(fileName);
											char tempLen[2];
											memset(tempLen, 0, 2);
											char msgFile[256];
											memset(msgFile, 0, 256);
											if(lenFileName < 10)
											{
												strcat(msgFile, "0");
											}
											sprintf(tempLen, "%d", lenFileName);
											strcat(msgFile, tempLen);
											strcat(msgFile, fileName);
											//strcat(fileName, "|");
											send(socket, msgFile, strlen(msgFile), 0);
											long fileSize;
											fseek (pFile , 0 , SEEK_END);
											fileSize = ftell (pFile);
											rewind (pFile);
											printf("lSize = %ld\n",fileSize);
											char fileLen[8];
											char tempfileLen[8];
											memset(tempfileLen, 0, 8);
											memset(fileLen, 0, 8);
											if(fileSize < 10)
												strcat(fileLen, "0000000");
											else if(fileSize < 100)
												strcat(fileLen, "000000");
											else if(fileSize < 1000)
												strcat(fileLen, "00000");
											else if(fileSize < 10000)
												strcat(fileLen, "0000");
											else if(fileSize < 100000)
												strcat(fileLen, "000");
											else if(fileSize < 1000000)
												strcat(fileLen, "00");
											else if(fileSize < 10000000)
												strcat(fileLen, "0");
											sprintf(tempfileLen, "%ld", fileSize);
											strcat(fileLen, tempfileLen);
											send(socket, fileLen, strlen(fileLen), 0);
											while(fgets (p2p_Buf , BUF_SIZE , pFile) != NULL)
											{
												int total = 0;
												int bytesleft = strlen(p2p_Buf);
												int n;
												while(total < strlen(p2p_Buf))
												{
													n = send(socket, p2p_Buf + total, bytesleft, 0);
													if(n == -1)
													{
														break;
													}
													total += n;
													bytesleft -= n;
												}
											}
											free(p2p_Buf);
											fclose(pFile);
											close(socket);
											cse4589_print_and_log("[SENDFILE:SUCCESS]\n");
											cse4589_print_and_log("[SENDFILE:END]\n");
										}
										else
										{
											cse4589_print_and_log("[SENDFILE:ERROR]\n");
											cse4589_print_and_log("[SENDFILE:END]\n");
										}
									}
								}
							}
							
						}
						else
						{
							cse4589_print_and_log("[%s:ERROR]\n",command_str);
							cse4589_print_and_log("[%s:END]\n",command_str);
						}
					}
				}
				else if(i == p2p_skt)
				{
					struct sockaddr_in remote_address;
					socklen_t len = sizeof(remote_address);
					int rem_skt;
					if((rem_skt = accept(p2p_skt, (struct sockaddr *)&remote_address, &len)) < 0)
					{
						printf("accept failed %d", errno);
						break;
					}

					FILE * pFile;
					int recv_len;
					char p2p_Buf[256];
					char fileName[25];
					memset(p2p_Buf, 0,256 );
					memset(fileName, 0, 25 );
					int lenCount = 2;
					char msgLen[2];
					//pFile = fopen ("myfile.txt","w");
					//if (pFile!=NULL)
					{
						recv_len = recv(rem_skt, msgLen, 2, 0);
						int lenFileName = atoi(msgLen);
						recv_len = recv(rem_skt, fileName, lenFileName, 0);
						pFile = fopen (fileName, "wb");
						//pFile = fopen ("myfile.txt", "wb");
						char fileLen[8];
						memset(fileLen, 0, 8);
						recv_len = recv(rem_skt, fileLen, 8, 0);
						int fileSize = atoi(fileLen);
						int total = 0;
						int n;
						while(total < fileSize)
						{
							n = recv(rem_skt, p2p_Buf, 256, 0);
							total += n;
							fwrite(p2p_Buf, 1, n, pFile);
							if(total == fileSize)
								break;
						}
						/*while((recv_len = recv(rem_skt, p2p_Buf, 256, 0)) > 0)
						{
							//strtok(p2p_Buf, '\0');
							fwrite(p2p_Buf, 1, recv_len, pFile);
						}*/
						fclose(pFile);
					}
					close(rem_skt);
				}
				else if(i == socket)
				{
					bool bCloseSock = false;
					bool bBreakLoop = false;
					char tempBuf[BUF_SIZE];
					int len;
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
								}
							}
						}*/
						int lenCount = 3;
						char msgLen[3];
						len = recv(socket, msgLen, 3, 0);
						int total1 = 0;
						int lenMsg = atoi(msgLen);
						int bytesleft1 = lenMsg;
						int n;
						printf("length = %d\n", lenMsg);
						while(total1 < lenMsg)
						{
							n = recv(socket, buffer + total1, bytesleft1, 0);
							total1 += n;
							bytesleft1 -= n;
							printf("buffer = %s\n", buffer);
							if(total1 == lenMsg)
								break;
							/*if(n == 0)
							{
								close(socket);
								FD_CLR(socket, &master_list);
								break;
							}*/
						}
						//memset(tempBuf, 0, BUF_SIZE);
						fflush(stdout);
						if(0 == n)
						{
							close(socket);
							FD_CLR(socket, &master_list);
							break;
						}
						/*else if(strstr(buffer, "|"))
						{
							bBreakLoop = true;
						}*/
						char* temp = strstr(buffer, "LoggedIn ");
						if(temp)
						{
							parseClientList(temp + 9);
						}
						else if(temp = strstr(buffer, "REFRESH "))
						{
							parseClientList(temp + 8);
							cse4589_print_and_log("[REFRESH:SUCCESS]\n");
							cse4589_print_and_log("[REFRESH:END]\n");
						}
						/*else if(temp = strstr(buffer, "Buffered "))
						{
							parseClientList(temp + 9);
						}*/
						else if(temp = strstr(buffer, "Endofmsg"))
						{
							bLoggedIn = true;
							cse4589_print_and_log("[LOGIN:SUCCESS]\n");
							cse4589_print_and_log("[LOGIN:END]\n");
						}
						else
						{
							//strcat(buffer, "&$");
							//strcpy(tempBuf, buffer);
							
							char* sender_IP = strtok(buffer, " ");
							char* msg = strtok(NULL, "\r\n");
							//char* data = strtok(NULL, "|");
							printf("msg = %s", msg);
							//while(data)
							{
								cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
								cse4589_print_and_log("msg from:%s\n[msg]:%s\n", sender_IP, msg);
								cse4589_print_and_log("[RECEIVED:END]\n");
								//sender_IP = strtok(NULL, " ");
								//sender_IP = sender_IP + 2;
								/*if(sender_IP)
								{
									data = strtok(NULL, "|");
									if(data && data[strlen(data) - 1] == '$' && data[strlen(data) - 2] == '&')
										break;
								}
								else
								{
									data = NULL;
								}*/
							}
						}
							
						if(bBreakLoop)
						{
							break;
						}
					}
				}
			}
		}

	}
	return 0;
}

int runClient(char* serverIP, int serverPort)
{
	int network_socket = socket(AF_INET, SOCK_STREAM, 0);
	char *port_msg = (char*)malloc(BUF_SIZE/8);
	char* temp_msg = (char*)malloc(BUF_SIZE/8);
	memset(port_msg, 0, BUF_SIZE/8);
	memset(temp_msg, 0, BUF_SIZE/8);
	int client_port = getListenPort();
	char port[10];
	sprintf(port, "%d", client_port);

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(serverPort);
	int ret = inet_pton(AF_INET, serverIP, &server_address.sin_addr);
	if(ret == 0)
	{
		printf("inet_pton failed!!\n");
		fflush(stdout);
		return 0;
	}
	printf("Connecting to server");
	int connection_status = connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address));
	//check for error with connection
	if(connection_status == -1)
	{
		printf("THere was an error\n\n");
		fflush(stdout);
		return 0;
        }
	bLoggedIn = true;
	strcat(temp_msg, "PORT ");
	strcat(temp_msg, port);
	//strcat(temp_msg, "|");
	int msgLen = strlen(temp_msg);
	char tempLen[3];
	memset(tempLen, 0, 3);
	if(msgLen < 10)
		strcat(port_msg, "00");
	else if(msgLen < 100)
		strcat(port_msg, "0");
	sprintf(tempLen, "%d", msgLen);
	strcat(port_msg, tempLen);
	strcat(port_msg, temp_msg);
	send(network_socket, port_msg, strlen(port_msg), 0);
	free(port_msg);
	free(temp_msg);
	return network_socket;
}

int runClientP2P(char* remoteIP, int remotePort)
{
	int network_socket = socket(AF_INET, SOCK_STREAM, 0);
	
	struct sockaddr_in remote_address;
	remote_address.sin_family = AF_INET;
	remote_address.sin_port = htons(remotePort);
	int ret = inet_pton(AF_INET, remoteIP, &remote_address.sin_addr);
	if(ret == 0)
	{
		printf("inet_pton failed!!\n");
		fflush(stdout);
		return 0;
	}
	int connection_status = connect(network_socket, (struct sockaddr *) &remote_address, sizeof(remote_address));
	if(connection_status == -1)
	{
		printf("THere was an error\n\n");
		fflush(stdout);
		return 0;
	}
	return network_socket;
}

int runServerP2P()
{
	int p2p_socket;
	struct sockaddr_in p2p_address;
	if(((p2p_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0))
	{
		printf("socket failed %d", errno);
		return 0;
	}
	p2p_address.sin_family = AF_INET;
	p2p_address.sin_port = htons(getListenPort());
	p2p_address.sin_addr.s_addr = INADDR_ANY;
	
	if((bind(p2p_socket, (struct sockaddr*) &p2p_address, sizeof(p2p_address))) < 0)
	{
		printf("bind failed %d", errno);
		return 0;
	}
	
	if((listen(p2p_socket, 5)) < 0)
	{
		printf("listen failed %d", errno);
	}

	return p2p_socket;
}
