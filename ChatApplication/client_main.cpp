#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <vector>
#include <string>
#include <iostream>

#include "ReqRegister.pb.h"
#include "ReqLogin.pb.h"
#include "ResLogin.pb.h"
#include "ResRegister.pb.h"
#include "MessageEncapsule.pb.h"

//#include "ResRegister.pb.h"


#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 32						
#define DEFAULT_PORT "27016"	
#define SERVER "127.0.0.1"						

enum CreateAccountWebResult
{
	SUCCESS,
	ACCOUNT_ALREADY_EXIST,
	INVALID_PASSWORD,
	INTERNAL_SERVER_ERROR,
	EMAIL_NOT_FOUND
};

enum createAccount {
	reqID_create = 143,
	reqID_create_success = 666,
	reqID_create_fail = -1,
};

enum login {
	reqID_login = 134,
	reqID_login_success = 999,
	reqID_login_fail = -2,
};

int main(int argc, char** argv)
{
	WSADATA wsaData;
	SOCKET connectSocket = INVALID_SOCKET;

	struct addrinfo* infoResult = NULL;
	struct addrinfo* ptr = NULL;
	struct addrinfo hints;

	std::string Name;
	int choice;

	char recvbuf[DEFAULT_BUFLEN];
	char recvingbuf[DEFAULT_BUFLEN];
	int result;
	int recvbuflen = DEFAULT_BUFLEN;
	int recvingbuflen = DEFAULT_BUFLEN;

	bool quit = false;
	bool isConnected = true;
	std::vector<int> sInput;
	int sendResult;
	char getnew[100];
	bool isLoggedIn = false;

	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		printf("WSAStartup failed with error: %d\n", result);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	result = getaddrinfo(SERVER, DEFAULT_PORT, &hints, &infoResult);
	if (result != 0)
	{
		printf("getaddrinfo failed with error: %d\n", result);
		WSACleanup();
		return 1;
	}



	for (ptr = infoResult; ptr != NULL; ptr = ptr->ai_next)
	{
		connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (connectSocket == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		result = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (result == SOCKET_ERROR)
		{
			closesocket(connectSocket);
			connectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}


	freeaddrinfo(infoResult);

	if (connectSocket == INVALID_SOCKET)
	{
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	protoAuth::Registeration* registration_req = new protoAuth::Registeration();
	protoAuth::Login* login_request = new protoAuth::Login();
	protoAuth::LoginRespose* login_res = new protoAuth::LoginRespose();
	protoAuth::RegisterationRespose* registration_res = new protoAuth::RegisterationRespose();
	protoAuth::MessageEncapsule* message_Encap = new protoAuth::MessageEncapsule();

	while (!quit) {

		if (_kbhit())
		{

			char  key = _getch();

			if (key == 27)                //ESC
			{
				quit = true;
			}

			if (key == 8)                //Backspace
			{
				printf("\b \b");
			}

			if (key == 13)                //Enter
			{
				bool checkRegistered = false;
				bool createAcount = false;
				bool loggedIn = false;
				//bool RegPasswordGot = false;
				bool RegUsernameGot = false;
				bool RegPasswordGot = false;

				std::string s = "";
				std::string GetMessageGroup = "";
				std::string RegUsername = "";
				std::string RegPassword = "";

				for (int i = 0; i < sInput.size(); i++) {
					s = s + recvbuf[i];

					if ((s == "/register") || (s == "\r/register"))
					{
						//std::cout << "came in register";
						checkRegistered = true;
						s = "";
						i++;
						createAcount = true;
						continue;
					}

					if ((s == "/authenticate") || (s == "\r/authenticate"))
					{
						//std::cout << "came in register";
						checkRegistered = true;
						s = "";
						i++;
						loggedIn = true;
						continue;
					}

					if (checkRegistered) {
						if (RegUsernameGot)
						{
							if (recvbuf[i] == 'Ì')
							{
								//printf("empty2 \n");
								//RegPasswordGot = true;
								checkRegistered = false;
							}
							else {
								RegPassword = s;
							}
						}
						else
						{
							if (recvbuf[i] == ' ')
							{
								//printf("empty1 \n");
								RegUsernameGot = true;
								s = "";
								//i++;
								continue;
							}
							else {
								RegUsername = s;
							}
						}
					}
				}
				if (createAcount) {

					registration_req->set_id(reqID_create);
					registration_req->set_email(RegUsername);
					registration_req->set_password(RegPassword);
					std::string serializedString;
					registration_req->SerializeToString(&serializedString);

					const char* str = serializedString.c_str();
					message_Encap->set_type(message_Encap->LOGIN);
					message_Encap->set_data(str);
					std::string serialize;
					message_Encap->SerializeToString(&serialize);
					const char* stringChar = serialize.c_str();
					sendResult = send(connectSocket, serialize.c_str(), serialize.size(), 0);
				}
				else if (loggedIn) {
					login_request->set_id(reqID_login);
					login_request->set_email(RegUsername);
					login_request->set_password(RegPassword);
					std::string serializedString;
					login_request->SerializeToString(&serializedString);

					const char* str = serializedString.c_str();
					message_Encap->set_type(message_Encap->LOGIN);
					message_Encap->set_data(str);
					std::string serialize;
					message_Encap->SerializeToString(&serialize);
					const char* stringChar = serialize.c_str();
					sendResult = send(connectSocket, stringChar, serialize.size(), 0);
				}
				else {
					if (isLoggedIn) {
						message_Encap->set_type(message_Encap->OTHER);
						message_Encap->set_data(recvbuf);
						std::string serializedString;
						message_Encap->SerializeToString(&serializedString);
						const char* str = serializedString.c_str();
						sendResult = send(connectSocket, str, sInput.size(), 0);
					}
				}
				//login_request

				if (sendResult == SOCKET_ERROR)
				{
					printf("send failed with error: %d\n", WSAGetLastError());
				}

				printf("\n");
				sInput.clear();
			}
			sInput.push_back(key);
			printf("%c", key);

			for (unsigned int index = 0; index < sInput.size(); index++)
			{
				recvbuf[index] = sInput[index];
			}

		}

		if (isConnected)
		{
			u_long iMode = 1;
			result = ioctlsocket(connectSocket, FIONBIO, &iMode);
			if (result != NO_ERROR)
			{
				printf("ioctlsocket failed with error: %d\n", result);
			}

			
			result = recv(connectSocket, recvingbuf, recvingbuflen, 0);
			if (recvingbuf == "Login Successful") {
				isLoggedIn = true;
			}
			

			if (result > 0)
			{
				printf("%.*s\n", result, recvingbuf);
			}
			else if (result == 0)
			{
				printf("Connection closed\n");
				isConnected = false;
			}
			else if (WSAGetLastError() == 10035) {
				continue;
			}
			else
			{
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(connectSocket);
				WSACleanup();
				return 1;
			}
		}

	}


	printf("\n shutdown");
	result = shutdown(connectSocket, SD_SEND);
	if (result == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}

	closesocket(connectSocket);
	WSACleanup();

	system("pause");

	return 0;
}