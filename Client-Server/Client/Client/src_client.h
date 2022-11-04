#pragma once


#define WIN32_LEAN_AND_MEAN
#define BUFFER_SIZE 10
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable: 4996)

#include <iostream>
#include <Windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <string>
#include "openssl/sha.h"

using namespace std;

class Client {
	WSADATA wsaData;
	ADDRINFO hints;
	ADDRINFO* addrResult;
	SOCKET ConnectSocket;
	
	int result;
	//char buffer[BUFFER_SIZE];
	//char recvBuffer[BUFFER_SIZE];
	string cmd, IP, port;
	int buf_size, recvBuf_size;

	string buffer, _exit = "exit", login, pass;
	char* recvBuffer;
	
	unsigned char hash[32];
	const unsigned char* src_hash;

	int auth();

public:
	Client();
	~Client();
	void init_client();

	int header();
	void start();

};

void Client::start()
{
	init_client();
	header();
}

Client::Client()
{
	addrResult = NULL;
	ConnectSocket = INVALID_SOCKET;
}

Client::~Client()
{
}

int Client::auth()
{
	int size_srcHash = 0, size_login =0, size_pass =0;
	cout << "\nEnter the authorization data.";
	cout << "\nlogin: ";
	cin >> login;
	size_login = login.size() + 1;
	
	//send login
	result = send(ConnectSocket, (char*)&size_login, sizeof(int), NULL);
	cout << "\nSended " << result << " bytes" << endl;
	result = send(ConnectSocket, login.c_str(), size_login, NULL);
	cout << "Sended " << result << " bytes" << endl;

	cout << "\nPass: ";
	cin >> pass;
	size_pass = pass.size() + 1;

	//get hash for password
	src_hash = reinterpret_cast<const unsigned char*>(pass.c_str());
	cout << "\n\tHashing...\n";
	//SHA256(src_hash, size_srcHash, hash);
	//puts(reinterpret_cast<const char*>(hash));

	//send hash password
	result = send(ConnectSocket, (char*)&size_pass, sizeof(int), NULL);
	cout << "\nSended " << result << " bytes" << endl;
	result = send(ConnectSocket, pass.c_str(), size_pass, NULL);
	if (result == SOCKET_ERROR)
	{
		cout << "Send failed!" << endl;
		return 0;
	}
	else 
		cout << "Sended " << result << " bytes" << endl;

	//get answer from server
	recv(ConnectSocket, (char*)&recvBuf_size, sizeof(int), NULL);
	recvBuffer = new char[recvBuf_size];
	result = recv(ConnectSocket, recvBuffer, recvBuf_size, NULL);
	if (result == SOCKET_ERROR)
	{
		cout << "Recovered failed!" << endl;
		return 0;
	}
	if (recvBuffer == "successfully")
	{
		cout << "\nAuthorization was successful!";
		return 1;
	}
	else {
		cout << recvBuffer << endl;
		login.clear();
		delete[] recvBuffer;
		auth();
	}
}

int Client::header()
{
	/*if (auth() == false)
	{
		cout << "\nAuthorization error!" << endl;
		closesocket(ConnectSocket);
		freeaddrinfo(addrResult);
		WSACleanup();
		system("pause");
		exit(10);
	}*/

	cout << "\n:";
	cin.ignore();
	do {
		buffer.clear();
		getline(std::cin, buffer);
		buf_size = buffer.size();
		buffer[buf_size] == '\0';
		buf_size++;
		
		//если введена команда exit - закрываем соединение и выходим
		if (buffer == _exit)
		{
			if (shutdown(ConnectSocket, SD_BOTH) == SOCKET_ERROR)
			{
				cout << "Shutdown error!" << endl;;
				closesocket(ConnectSocket);
				freeaddrinfo(addrResult);
				WSACleanup();
				system("pause");
				exit(1);
			}
			return 0;
		}

		// -> отправляем размер буфера
		// -> отправляем сам буфер

		//cout << "\t...send your data...\n";
		send(ConnectSocket, (char*)&buf_size, sizeof(int), NULL);
		result = send(ConnectSocket, buffer.c_str(), buf_size, NULL);
		if (result == SOCKET_ERROR)
		{
			cout << "Send failed!" << endl;
			closesocket(ConnectSocket);
			freeaddrinfo(addrResult);
			WSACleanup();
			system("pause");
			exit(1);
		}


		//cout << "\n\t...getting data from server...\n";
		recv(ConnectSocket, (char*)&recvBuf_size, sizeof(int), NULL);
		recvBuffer = new char[recvBuf_size];
		result = recv(ConnectSocket, recvBuffer, recvBuf_size, NULL);
		if (result < recvBuf_size)
		{
			cout << "\n\nWAITING ALL DATA\n";
		}
		cout << "#" << recvBuffer << " ";
		if (recvBuffer == "disconnected")
			break;
		delete[] recvBuffer;
		if (result == SOCKET_ERROR)
			{
				cout << "Recovered failed!" << endl;
				closesocket(ConnectSocket);
				freeaddrinfo(addrResult);
				WSACleanup();
				system("pause");
				exit(1);
			}
	} while (true);
}

void Client::init_client()
{
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << "WSAStartup failed!";
		system("pause");
		exit(1);
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	cout << "Do you want to set the IP and port values manually or use standard values? [localhost, 1777]\n(y/ any key): ";
	cin >> IP;
	if (IP == "y" || IP == "Y")
	{
		cout << "Enter IP: ";
		cin >> IP;
		cout << "\nport: ";
		cin >> port;
	}
	else
	{
		IP = "localhost";
		port = "1777";
	}

	if (getaddrinfo(IP.c_str(), port.c_str(), &hints, &addrResult) != 0)
	{
		cout << "getaddrinfo() failed!";
		freeaddrinfo(addrResult);
		WSACleanup();
		system("pause");
		exit(1);
	}

	ConnectSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
	if (ConnectSocket == INVALID_SOCKET)
	{
		cout << "Socket creation failed" << endl;
		freeaddrinfo(addrResult);
		WSACleanup();
		system("pause");
		exit(1);
	}

	if (connect(ConnectSocket, addrResult->ai_addr, (int)addrResult->ai_addrlen) == SOCKET_ERROR)
	{
		cout << "Unable connect to server" << endl;
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
		freeaddrinfo(addrResult);
		WSACleanup();
		system("pause");
		exit(1);
	}
	else {
		cout << "\n\tSuccessfully connected to the server " << ConnectSocket << endl;
	}
}