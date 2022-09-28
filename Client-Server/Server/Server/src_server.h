#pragma once
#define WIN32_LEAN_AND_MEAN
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable: 4996)

#include <iostream>
#include <Windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <string>
#include <array>
#include <thread>


//user levels
#define USER "user"
#define ADMIN "admin"
#define GOD "god"
#define WORM "wrom"
//user groups
#define NONE "none"

#define BUFFER_SIZE 10


using namespace std;

class Server
{
private:
	//структура в которой описаны поля статуса юзера, его группа, ip c которого он был создан
	typedef struct {
		string type_user = USER,
			group = NONE,
			IP = NONE;
	} USERS[40];
	//матрица прав пользователей
	array<array<string, 50>, 50> LawUser;

	WSADATA wsaData;
	ADDRINFO hints;
	ADDRINFO* addrResult;
	SOCKET ClientSocket[20], ListenSocket;

	thread *th[20];

	int result, counter, buf_size, recvBuf_size;
	char *buffer;
	char *recvBuffer;
	string cmd, IP, port, _buffer, _recvBuffer;
public:
	Server();
	~Server();

	void init();
	//основная функция в которой бежит вечный цикл 
	void header();
	//получить сведения о правах юзера на папку/файл
	int get_access();
	int set_access();
	//получить сведения о пользователе (его группа, статус)
	int set_user();
	int get_user();

	int send_data();
	void Client_header();
	void static thread_starter(LPVOID that, int param);

	void start();
};

void Server::start()
{
	init();
	header();
}

Server::Server()
{
	buf_size = 0;
	recvBuf_size = 0;
	counter = 0;
	addrResult = NULL;
	ListenSocket = INVALID_SOCKET;
};

Server::~Server()
{
	delete[] recvBuffer;
	delete[] buffer;
}

void Server::init()
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
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, "1777", &hints, &addrResult) != 0)
	{
		cout << "getaddrinfo() failed!";
		freeaddrinfo(addrResult);
		WSACleanup();
		system("pause");
		exit(1);
	}

	ListenSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
	if (ListenSocket == INVALID_SOCKET)
	{
		cout << "Socket creation failed!" << endl;
		freeaddrinfo(addrResult);
		WSACleanup();
		system("pause");
		exit(1);
	}

	if (bind(ListenSocket, addrResult->ai_addr, (int)addrResult->ai_addrlen) == SOCKET_ERROR)
	{
		cout << "Binding socket failed!" << endl;
		ListenSocket = INVALID_SOCKET;
		closesocket(ListenSocket);
		freeaddrinfo(addrResult);
		WSACleanup();
		system("pause");
		exit(1);
	}

	if (listen(ListenSocket, 1) == SOCKET_ERROR)
	{
		cout << "Listening socket failed!" << endl;
		ListenSocket = INVALID_SOCKET;
		closesocket(ListenSocket);
		freeaddrinfo(addrResult);	
		WSACleanup();
		system("pause");
		exit(1);
	}
}

void Server::header()
{
	counter++;
	ClientSocket[counter] = accept(ListenSocket, NULL, NULL);
	if (ClientSocket[counter] == INVALID_SOCKET)
	{
		cout << "Accepted error!" << endl;
		closesocket(ClientSocket[0]);
		freeaddrinfo(addrResult);
		WSACleanup();
		system("pause");
		exit(1);
	}
	cout << "\nClient " << addrResult->ai_addr << " connected\n";

	th[counter]->join();
	//CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)thread_starter, this, NULL, NULL);
	//Client_header();
}

void Server::thread_starter(LPVOID that, int param)
{
	return ((Server*)that)->Client_header();
}

void Server::Client_header()
{
	do {
		cout << "\n\t...getting data from clent :: " << Server::addrResult->ai_addr << endl;
		
		//SecureZeroMemory(recvBuffer, recvBuf_size);
		recv(ClientSocket[counter], (char*)&recvBuf_size, sizeof(int), NULL);
		char* recvBuffer = new char[recvBuf_size];
		result = recv(ClientSocket[counter], recvBuffer, recvBuf_size, NULL);
			
		if (result == SOCKET_ERROR)
		{
			cout << "getting failed!" << endl;
			closesocket(ClientSocket[counter]);
			freeaddrinfo(addrResult);
			WSACleanup();
			system("pause");
			exit(1);
		}

		cout << recvBuffer << endl;
		send_data();
		delete[] recvBuffer;
	} while (true);
}

int Server::send_data()
{
	//pars_command();
	_buffer = "Comman for clent!";

	//отправляем данные по запросу клиента
	cout << "\t...send data to client...\n";

	// -> отправляем размер буфера
	// -> отправляем сам буфер

	buf_size = _buffer.size();
	_buffer[buf_size] = '\0';
	buf_size++;
	send(ClientSocket[counter], (char*)&buf_size, sizeof(int), NULL);
	result = send(ClientSocket[counter], _buffer.c_str(), buf_size, NULL);
	if (result == SOCKET_ERROR)
	{
		cout << "Send failed!" << endl;
		closesocket(ClientSocket[0]);
		freeaddrinfo(addrResult);
		WSACleanup();
		system("pause");
		exit(1);
	}
}