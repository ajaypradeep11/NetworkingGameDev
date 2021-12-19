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
#include "HashGen.h"

#include "DBHelper.h"
#include "ReqRegister.pb.h"
#include "ResRegister.pb.h"
#include "ReqLogin.pb.h"
#include "ResLogin.pb.h"
#include "MessageEncapsule.pb.h"

DBHelper database;

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 32
#define DEFAULT_PORT "27077"

protoAuth::Registeration* registerationReq = new protoAuth::Registeration();
protoAuth::RegisterationRespose* registerationRes = new protoAuth::RegisterationRespose();
protoAuth::Login* loginReq = new protoAuth::Login();
protoAuth::LoginRespose* loginRes = new protoAuth::LoginRespose();
protoAuth::MessageEncapsule* messageEncap = new protoAuth::MessageEncapsule();

enum MessageResult
{
	REGISTER,
	LOGIN,
	LOGINRESPONSE,
	REGISTERRESPONSE,
	OTHER
};

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


std::string getSalt()
{
	std::string salt = "";
	

	float low = 1; 
	float high = 10;
	

	for (int i = 0; i < 8; i++)
	{
		int value = low + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / (high - low));
		salt += std::to_string(value);
		std::cout << salt << "\n";
	}
	
	return salt;
}

std::string getHashedPassword(const std::string& salt, const std::string& plainPassword)
{
	std::string password = plainPassword + salt;
	SHA256 sha;
	sha.update(password);
	uint8_t* digest = sha.digest();
	return SHA256::toString(digest);
}


int main(int argc, char** argv)
{
	database.Connect("127.0.0.1:3306", "root", "ajay1234");

	std::map<std::string, std::vector<SOCKET>>::iterator it;
	std::vector<SOCKET>::iterator socketIterator;

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

	iResult = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &addrResult);
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

				std::string s = "";

				messageEncap->ParseFromString(client->dataBuf.buf);

				if (messageEncap->type() == LOGIN) {
					loginReq->ParseFromString(messageEncap->data());
					if (loginReq->id() == 143) {
						loginReq->ParseFromString(messageEncap->data());
						std::string useremail = loginReq->email();
						std::string userpass = loginReq->password();
						std::cout << " login page " << useremail << "\n";
						std::cout << " login page " << userpass << "\n";


						std::cout << "buffer size :" << client->dataBuf.len << "\n";

						registerationReq->ParseFromString(messageEncap->data());
						/*std::string useremail = registerationReq->email();
						std::string userpass = registerationReq->password();*/
						std::string SaltPassword = getSalt();
						std::string HashedPassword = getHashedPassword(SaltPassword, userpass);
						std::cout << "usernae :" << useremail << "\n";
						std::cout << "usernae :" << userpass << "\n";
						std::cout << "usernae :" << SaltPassword << "\n";
						std::cout << "usernae :" << HashedPassword << "\n";

						CreateAccountWebResult value = database.CreateAccount(useremail, userpass, SaltPassword, HashedPassword);
						registerationRes->set_reqid(9);
						if (value == SUCCESS) {
							registerationRes->set_result(registerationRes->SUCCESS);
						}
						else if (value == ACCOUNT_ALREADY_EXIST) {
							registerationRes->set_result(registerationRes->ACCOUNT_ALREADY_EXISTS);
						}
						else if (value == PASSWORD_CONDITION_CHECK) {
							registerationRes->set_result(registerationRes->PASSWORD_CONDITION_CHECK);
						}
						else if (value == INTERNAL_SERVER_ERROR) {
							registerationRes->set_result(registerationRes->INTERNAL_SERVER_ERROR);
						}

						std::string serializedString;
						registerationRes->SerializeToString(&serializedString);
						const char* str = serializedString.c_str();

						messageEncap->set_type(messageEncap->REGISTERRESPONSE);
						messageEncap->set_data(str);
						std::string serialize;
						messageEncap->SerializeToString(&serialize);
						const char* stringChar = serialize.c_str();

						send(acceptSocket, stringChar, serialize.size(), 0);
					}
					else if (loginReq->id() == 134) {
						
						std::string useremail = loginReq->email();
						std::string userpass = loginReq->password();
						std::cout << " login page " << useremail << "\n";
						std::cout << " login page " << userpass << "\n";

						CreateAccountWebResult value = database.loginAccount(useremail, userpass);
						loginRes->set_reqid(8);
						if (value == SUCCESS) {
							loginRes->set_result(loginRes->SUCCESS);
						}
						else if (value == EMAIL_NOT_FOUND) {
							loginRes->set_result(loginRes->EMAIL_NOT_FOUND);
						}
						else if (value == INVALID_PASSWORD) {
							loginRes->set_result(loginRes->INVALID_PASSWORD);
						}
						else if (value == INTERNAL_SERVER_ERROR) {
							loginRes->set_result(loginRes->INTERNAL_SERVER_ERROR);
						}

						std::string serializedString;
						loginRes->SerializeToString(&serializedString);
						const char* str = serializedString.c_str();

						messageEncap->set_type(messageEncap->LOGINRESPONSE);
						messageEncap->set_data(str);
						std::string serialize;
						messageEncap->SerializeToString(&serialize);
						const char* stringChar = serialize.c_str();

						send(acceptSocket, stringChar, serialize.size(), 0);
					}
					
					
				}
								
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
