#ifndef TCPCLIENT_H_
#define TCPCLIENT_H_

int startClient(char* serverIP);
int runClient(char* serverIP, int serverPort);
int runClientP2P(char* remoteIP, int remotePort);
int runServerP2P();
#endif

