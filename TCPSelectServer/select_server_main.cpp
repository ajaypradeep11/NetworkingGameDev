#define WIN32_LEAN_AND_MEAN			

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <map>
#include <vector>

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 32
#define DEFAULT_PORT "27016"

// Client structure
struct ClientInfo {
	SOCKET socket;
	WSABUF dataBuf;
	char buffer[DEFAULT_BUFLEN];
	int bytesRECV;
	std::string group;
	std::string name;
	std::string toJoin;
	std::string toLeave;
	std::string Message;
};

int TotalClients = 0;
ClientInfo* ClientArray[FD_SETSIZE];

std::map<std::string, std::vector<SOCKET>> chatRooms;
std::vector<SOCKET> socketVector;


void RemoveClient(int index)
{
	ClientInfo* client = ClientArray[index];
	closesocket(client->socket);
	printf("Closing socket %d\n", (int)client->socket);

	for (int clientIndex = index; clientIndex < TotalClients; clientIndex++)
	{
		ClientArray[clientIndex] = ClientArray[clientIndex + 1];
	}

	TotalClients--;

}

int main(int argc, char** argv)
{
	std::map<std::string, std::vector<SOCKET>>::iterator it;
	std::vector<SOCKET>::iterator socketIterator;
	chatRooms.insert(std::pair<std::string, std::vector<SOCKET>>("Graphics",socketVector));
	chatRooms.insert(std::pair<std::string, std::vector<SOCKET>>("Networking", socketVector));
	chatRooms.insert(std::pair<std::string, std::vector<SOCKET>>("Physics", socketVector));
	chatRooms.insert(std::pair<std::string, std::vector<SOCKET>>("Gamepattern", socketVector));
	chatRooms.insert(std::pair<std::string, std::vector<SOCKET>>("MediaFun", socketVector));

	WSADATA wsaData;
	int iResult;
	WSABUF charac;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
	else
	{
		printf("WSAStartup() was successful!\n");
	}

	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET acceptSocket = INVALID_SOCKET;

	struct addrinfo* addrResult = NULL;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &addrResult);
	if (iResult != 0)
	{
		printf("getaddrinfo() failed with error %d\n", iResult);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("getaddrinfo() is good!\n");
	}

	listenSocket = socket(
		addrResult->ai_family,
		addrResult->ai_socktype,
		addrResult->ai_protocol
	);
	if (listenSocket == INVALID_SOCKET)
	{
		printf("socket() failed with error %d\n", WSAGetLastError());
		freeaddrinfo(addrResult);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("socket() is created!\n");
	}

	iResult = bind(
		listenSocket,
		addrResult->ai_addr,
		(int)addrResult->ai_addrlen
	);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(addrResult);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("bind() is good!\n");
	}

	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen() failed with error %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("listen() was successful!\n");
	}
	DWORD NonBlock = 1;
	iResult = ioctlsocket(listenSocket, FIONBIO, &NonBlock);
	if (iResult == SOCKET_ERROR)
	{
		printf("ioctlsocket() failed with error %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	printf("ioctlsocket() was successful!\n");

	FD_SET ReadSet;
	int total;
	DWORD flags;
	DWORD RecvBytes;
	DWORD SentBytes;

	printf("Entering accept/recv/send loop...\n");
	while (true)
	{
		timeval tv = { 0 };
		tv.tv_sec = 2;
		FD_ZERO(&ReadSet);
		FD_SET(listenSocket, &ReadSet);
		for (int i = 0; i < TotalClients; i++)
		{
			FD_SET(ClientArray[i]->socket, &ReadSet);
		}

		printf("Waiting for select()...\n");
		total = select(0, &ReadSet, NULL, NULL, &tv);
		if (total == SOCKET_ERROR)
		{
			printf("select() failed with error: %d\n", WSAGetLastError());
			return 1;
		}
		else
		{
			printf("select() is successful!\n");
		}

		//Receive and Send
		if (FD_ISSET(listenSocket, &ReadSet))
		{
			total--;
			acceptSocket = accept(listenSocket, NULL, NULL);
			if (acceptSocket == INVALID_SOCKET)
			{
				printf("accept() failed with error %d\n", WSAGetLastError());
				return 1;
			}
			else
			{
				iResult = ioctlsocket(acceptSocket, FIONBIO, &NonBlock);
				if (iResult == SOCKET_ERROR)
				{
					printf("ioctsocket() failed with error %d\n", WSAGetLastError());
				}
				else
				{
					printf("ioctlsocket() success!\n");

					ClientInfo* info = new ClientInfo();
					info->socket = acceptSocket;
					info->bytesRECV = 0;
					ClientArray[TotalClients] = info;
					TotalClients++;
					printf("New client connected on socket %d\n", (int)acceptSocket);
				}
			}
		}

		for (int i = 0; i < TotalClients; i++)
		{
			ClientInfo* client = ClientArray[i];
			if (FD_ISSET(client->socket, &ReadSet))
			{
				total--;
				client->dataBuf.buf = client->buffer;
				client->dataBuf.len = DEFAULT_BUFLEN;
			    charac.buf = client->buffer;
				DWORD Flags = 0;
				iResult = WSARecv(
					client->socket,
					&(client->dataBuf),
					1,
					&RecvBytes,
					&Flags,
					NULL,
					NULL
				);

				bool checkCondition = true;
				bool checkIfName = false;
				bool checkIfMessage = false;
				bool checkIfLeave = false;
				bool checkIfJoin = false;
				bool checkOutMessage = false;
				bool checkOutLeave = false;
				bool checkOutJoin = false;
				bool dataFound = false;
				int count = 0;

				std::string s = "";
				std::string GetMessageGroup = "";
				for (i = 0; i < client->dataBuf.len; i++) {
					s = s + client->buffer[i];
					if ((s == "/connect")|| (s == "\r/connect") && checkCondition) {
						std::cout << "came in 1";
						checkCondition = false;
						checkIfName = true;
						s = "";
						i++;
					}
					else if ((s == "/join")||(s == "\r/join")&& checkCondition) {
						std::cout << "came in 2";
						checkCondition = false;
						checkIfJoin = true;
						s = "";
						i++;
					}
					else if ((s == "/message")|| (s == "\r/message") && checkCondition) {
						std::cout << "came in 3";
						checkCondition = false;
						checkIfMessage = true;
						s = "";
						i++;
					}
					else if ((s == "/leave")||(s == "\r/leave")&& checkCondition) {
						std::cout << "came in 4";
						checkCondition = false;
						checkIfLeave = true;
						s = "";
						i++;
					}
					if (checkIfMessage) {
						if ((s == "Networking") || (s == "Graphics") || (s == "Media") || (s == "Physics")) {
							GetMessageGroup = s;
							s = "";
							i++;
							checkOutMessage = true;
						}
					}
					if (checkIfJoin) {
						count++;
						if ((s == "Networking") || (s == "Graphics") || (s == "Media") || (s == "Physics")) {
							checkOutJoin = true;
							break;
						}

					}if (checkIfLeave) {
						count++;
						if ((s == "Networking") || (s == "Graphics") || (s == "Media") || (s == "Physics")) {
							checkOutLeave = true;
							break;
						}
					}
				}

				if (checkIfName) {
				
					for (int i = s.length() - 1; i > 0; i--) {
						if (s[i] != '\0') {
							client->dataBuf.len = i;
							break;
						}
					}
					client->name = s.substr(0, client->dataBuf.len + 1);
					std::cout << "\n" << client->name;
				}else  if (checkOutJoin) {
					
					client->toJoin = s.substr(0, count-1);
					std::cout << "\n"<<client->toJoin;

					for (it = chatRooms.begin(); it != chatRooms.end(); it++)
					{
						std::string a = it->first;
						std::string b = client->toJoin;

						if (a.compare(b) == 0) {
							it->second.push_back(client->socket);
							client->group = s.substr(0, count - 1);
							break;
						}
						std::cout << it->first << "over"<< std::endl;
						std::cout << client->toJoin << "over" << std::endl;
					}

					for (it = chatRooms.begin(); it != chatRooms.end(); it++)
					{
						char ph[32];
						std::string name1 = " has joined the room";
						std::string name = client->name;
						std::string joiningword = name.append(name1);

						for (int i = 0; i < joiningword.length(); i++) {
							ph[i] = joiningword[i];
						}
						if ((it->first == client->group)) {
							for (int i = 0; i < it->second.size(); i++) {
								send(it->second[i], ph, joiningword.length(), 0);
							}
						}
					}

				}else  if (checkOutLeave) {
					client->toLeave = s.substr(0, count-1);
					std::cout << "\n" << client->toLeave;
					for (it = chatRooms.begin(); it != chatRooms.end(); it++)
					{
						char sh[32];
						std::string name1 = " has Left the room";
						std::string name = client->name;
						std::string joiningword = name.append(name1);

						for (int i = 0; i < joiningword.length(); i++) {
							sh[i] = joiningword[i];
						}
						if ((it->first == client->toLeave)) {
							for (int i = 0; i < it->second.size();i++) {
								if (it->second[i] == client->socket) {
									for (int j = 0; j < it->second.size(); j++) {
										checkOutLeave = false;
										send(it->second[j], sh, joiningword.length(), 0);
									}
									it->second.erase(it->second.begin() + i); 
									client->group = "";
									dataFound = true;
									break;
									
								}
							}
						}
					}

				}else  if (checkOutMessage) {
					client->Message = s.substr(0, client->dataBuf.len - 1);
					std::cout << "\n" << client->Message;
					char ph[60];

					for (int i = 0; i < client->Message.length();i++) {
						ph[i] = client->Message[i];
					}
					for (it = chatRooms.begin(); it != chatRooms.end(); it++)
					{
						if ((it->first == GetMessageGroup)) {
							for (int i = 0; i < it->second.size(); i++) {
								if (it->second[i] == client->socket) {
									for (int j = 0; j < it->second.size(); j++) {
										send(it->second[j], ph, DEFAULT_BUFLEN, 0);
									}
									
								}
								
							}
						}
					}
				}
				else {
					continue;
				}

				if((!dataFound)&&(checkOutLeave) )
					send(client->socket, "You are not in the Group", DEFAULT_BUFLEN, 0);

				count = 0;


				if (iResult == SOCKET_ERROR)
				{
					if (WSAGetLastError() == WSAEWOULDBLOCK)
					{
						// We can ignore this, it isn't an actual error.
					}
					else
					{
						printf("WSARecv failed on socket %d with error: %d\n", (int)client->socket, WSAGetLastError());
						RemoveClient(i);
					}
				}
				else
				{
					printf("WSARecv() is OK!\n");
					if (RecvBytes == 0)
					{
						RemoveClient(i);
					}
					else if (WSAGetLastError() == 10054) {

					}
					else if (RecvBytes == SOCKET_ERROR)
					{
						printf("recv: There was an error..%d\n", WSAGetLastError());
						continue;
					}
					else
					{

					
					}
				}
			}
		}

	}



	// #6 close
	iResult = shutdown(acceptSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(acceptSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(acceptSocket);
	WSACleanup();

	return 0;
}