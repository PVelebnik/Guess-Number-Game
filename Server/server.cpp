#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <cctype>
#include <set>
#include <vector>
#include <algorithm>
#include <iterator>
// Need to link with Ws2_32.lib
#pragma comment(lib,"ws2_32")
//#pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

int __cdecl main(void)
{
	std::string number;


	std::cout << "Enter number 4 digits :";
	std::getline(std::cin, number);
	if (number.size() != 4)
	{
		return -1;
	}

	for (auto c : number)
	{
		if (!std::isdigit(c))
		{
			return -1;
		}
	}

	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN] = {};
	int recvbuflen = DEFAULT_BUFLEN;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	for (;;)
	{
		iResult = listen(ListenSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR) {
			printf("listen failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}


		// Receive until the peer shuts down the connection
		do
		{
			ZeroMemory(recvbuf, recvbuflen);
			iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
			if (iResult > 0) {
				std::cout << "Message from client : " << recvbuf << std::endl;

				std::string number_from_server = recvbuf;

				bool is_number = true;
				if (number_from_server.size() != 4)
				{
					is_number = false;
				}

				if (is_number)
				{
					for (auto c : number_from_server)
					{
						if (!std::isdigit(c))
						{
							is_number = false;
						}
					}
				}


				if (!is_number)
				{
					const std::string answer = "Number has wrong format!";
					iSendResult = send(ClientSocket, answer.c_str(), answer.size(), 0);
					if (iSendResult == SOCKET_ERROR) {
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(ClientSocket);
						WSACleanup();
						continue;
					}
				}
				else
				{
					std::set<char> set_number(number.begin(), number.end());
					std::set<char> set_number_from_server(number_from_server.begin(),
						number_from_server.end());

					if (set_number_from_server.size() != 4)
					{
						const std::string answer = "Has same digits!";
						iSendResult = send(ClientSocket, answer.c_str(), answer.size(), 0);
						if (iSendResult == SOCKET_ERROR) {
							printf("send failed with error: %d\n", WSAGetLastError());
							closesocket(ClientSocket);
							WSACleanup();
							return -1;
						}
						continue;
					}


					if (number == number_from_server)
					{
						const std::string answer = "VICTORY!";
						iSendResult = send(ClientSocket, answer.c_str(), answer.size(), 0);
						if (iSendResult == SOCKET_ERROR) {
							printf("send failed with error: %d\n", WSAGetLastError());
							closesocket(ClientSocket);
							WSACleanup();
							return -1;
						}
						continue;
					}
					else
					{
						std::vector<char> result;

						std::set_intersection(set_number.begin(),
							set_number.end(),
							set_number_from_server.begin(),
							set_number_from_server.end(), std::back_inserter(result));

						int correct_pos = 0;
						for (int i = 0; i < 4; i++)
						{
							if (number_from_server[i] == number[i])
							{
								correct_pos++;
							}
						}

						std::string msg = "Try more, you guess : ";
						msg += " " + std::to_string(result.size()) + " ";
						msg += " number on right position: " + std::to_string(correct_pos);

						iSendResult = send(ClientSocket, msg.c_str(), msg.size(), 0);
						if (iSendResult == SOCKET_ERROR) {
							printf("send failed with error: %d\n", WSAGetLastError());
							closesocket(ClientSocket);
							WSACleanup();
							return 1;
						}
					}
				}
			}
			else if (iResult == 0)
				printf("End session\n");
			else {
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				break;
			}
		} while (iResult > 0);


		// shutdown the connection since we're done
		iResult = shutdown(ClientSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
	}
	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

	return 0;
}
