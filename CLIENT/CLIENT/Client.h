#pragma once

#define WIN32_LEAN_AND_MEAN
#define BUFFER_SIZE 10
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib,"Crypt32.lib")
#pragma warning(disable: 4996)

#include <iostream>
#include <Windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <string>
#include "openssl/sha.h"

#define RECV_FL 0
#define SEND_FL 1
#define AUTH 0
#define REG 1

using namespace std;

class Client {
	WSADATA wsaData;
	ADDRINFO hints;
	ADDRINFO* addrResult;
	SOCKET ConnectSocket;


	string cmd, IP, port;
	int buf_size, recvBuf_size,
		val1, val2;

	string buffer, login, pass;
	char* recvBuffer;

	unsigned char hash[32];
	const unsigned char* src_hash;

	int auth(bool fl);

public:
	Client();
	~Client();
	void init_client();

	int header();
	void start();

	void close_connection(bool FL);
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

int Client::auth(bool FL)
{
	int size = 0;
	cout << "\nEnter the authorization data.";
	cout << "\nlogin: ";
	cin >> login;

	cout << "Pass: ";
	cin >> pass;

	unsigned char* _pass;
	unsigned char hash[64];
	int temp;


	size = pass.size();

	_pass = new unsigned char[size+1];
	for (int i = 0; i < pass.size(); i++)
		_pass[i] = pass[i];

	//get hash password
	SHA512(_pass, size, hash);
	/*cout << "HASH OF PASSWORD ==> ";
	for (int i = 0; i < 64; i++) 
		printf("%02x ", hash[i]);*/
	
	pass.clear();
	pass.reserve(150);
	if (FL == AUTH)
		pass = "auth " + login + " ";
	if (FL == REG)
		pass = "reg " + login + " ";
	char* buffer;
	for (int i = 0; i < 64; i++)
	{
		temp = hash[i];
		if (temp > 0 && temp < 10)
			buffer = new char[1];
		else if (temp > 10 && temp < 100)
			buffer = new char[2];
		else
			buffer = new char[3];
		itoa(temp, buffer, 10);
		pass += buffer;
	}
	pass += '\0';
	size = pass.size();
	val1 = send(ConnectSocket, (char*)&size, sizeof(int), NULL);
	val2 = send(ConnectSocket, pass.c_str(), size, NULL);
	if (val1 == SOCKET_ERROR || val2 == SOCKET_ERROR)
		close_connection(SEND_FL);

	//get answer from server
	val1 = recv(ConnectSocket, (char*)&recvBuf_size, sizeof(int), NULL);
	recvBuffer = new char[recvBuf_size];
	val2 = recv(ConnectSocket, recvBuffer, recvBuf_size, NULL);
	if (val1 == SOCKET_ERROR || val2 == SOCKET_ERROR)
		close_connection(RECV_FL);

	if (recvBuffer[0] != 'E')
	{
		cout << "\nAuthorization was successful!\n";
		cout << recvBuffer << " ";
		return 1;
	}
	else {
		cout << recvBuffer << endl;
		return 0;
	}
}

int Client::header()
{
	string answer;
	bool _auth = false;

	int switch_on = 0;
	cout << "1 - authorization\n2 - registration\n->";
	cin >> switch_on;
	system("cls");
	switch (switch_on)
	{
	case 1:
		while (!_auth)
			_auth = auth(AUTH);
		break;
	case 2:
		auth(REG);
		break;
	default:
		cout << "Incorrect menu item. Use the command 'reg' for registration\n or the 'auth' command for authorization." << endl;
		break;
	}

	cin.ignore();
	do {
		//получаем комманду от юзера со СПВ
		buffer.clear();
		getline(std::cin, buffer);
		buf_size = buffer.size();
		buffer[buf_size] == '\0';
		buf_size++;
		if (buffer == "cls")
		{
			system("cls");
			continue;
		}
		answer = buffer;
		int it = answer.find(' ');
		if (it == -1)
			it = answer.size();
		answer.erase(it);
		if (answer == "auth")
		{
			auth(AUTH);
			continue;
		}
		if (answer == "reg")
		{
			auth(REG);
			continue;
		}

		// -> отправляем размер буфера
		// -> отправляем сам буфер
		val1 = send(ConnectSocket, (char*)&buf_size, sizeof(int), NULL);
		val2 = send(ConnectSocket, buffer.c_str(), buf_size, NULL);
		if (val1 == SOCKET_ERROR || val2 == SOCKET_ERROR)
			close_connection(SEND_FL);

		val1 = recv(ConnectSocket, (char*)&recvBuf_size, sizeof(int), NULL);
		recvBuffer = new char[recvBuf_size];
		val2 = recv(ConnectSocket, recvBuffer, recvBuf_size, NULL);
		if (val1 == SOCKET_ERROR || val2 == SOCKET_ERROR)
			close_connection(RECV_FL);
		answer = recvBuffer;

		cout << recvBuffer << " ";
		if (answer == "disconnected")
		{
			shutdown(ConnectSocket, 2);
			closesocket(ConnectSocket);
			delete[] recvBuffer;
			cout << endl;
			return 0;
		}
		delete[] recvBuffer;
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
		cout << "('localhost')Enter IP: ";
		cin >> IP;
		cout << "\n(Attention! Valid range of ports: 100 - 8001)\nport: ";
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
		system("cls");
		cout << "\tSuccessfully connected to the server on port " << ConnectSocket << endl;
	}
}

void Client::close_connection(bool FL)
{
	if (FL == SEND_FL)
		cout << "Send failed!" << endl;
	if (FL == RECV_FL)
		cout << "Recovered failed!" << endl;
	shutdown(ConnectSocket, 2);
	closesocket(ConnectSocket);
	freeaddrinfo(addrResult);
	WSACleanup();
	system("pause");
	exit(1);
}