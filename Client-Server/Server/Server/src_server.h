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

#include <openssl/sha.h>


//user levels
#define USER "user"
#define ADMIN "admin"
#define GOD "god"
#define WORM "wrom"
//user groups
#define NONE "none"
#define PRIORITY "priority"

#define MAX_LOAD 5


using namespace std;

class Server
{
private:
	//структура в которой описаны поля статуса юзера, его группа, ip c которого он был создан
	typedef struct USERS {
		string type_user = USER,
			group = NONE,
			ID = NONE,
			login = NONE,
			password = NONE,
			home_path = NONE;
		bool auth_token = false,
			online = false;
		int cur_sock;
	};
	USERS user[MAX_LOAD];
	//матрица прав пользователей
	array<array<string, 50>, 50> LawUser;

	WSADATA wsaData;
	ADDRINFO hints;
	ADDRINFO* addrResult;
	SOCKET ClientSocket[MAX_LOAD] = {INVALID_SOCKET}, ListenSocket;

	thread th[MAX_LOAD];

	int result, counter, buf_size, recvBuf_size;
	char *recvBuffer;
	string  _buffer, command;

	string supported_commands[22] = { "rr", "chmod", "help", "open", "close", "crF", "delete", 
		"crD", "cd", "crGr", "delGr", "addUnG", "delUnG", "write", "read", "ls", "logout",
	"crUser", "delUser", "chPUser", "chLUser", "listUser"};

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
	int find_user(string login);
	int find_user(int id);
	int set_user();
	int get_user();

	int send_data(SOCKET &currentSocket, string cmd);
	void Client_header(SOCKET &currentSocket, int ind);
	void static thread_starter(LPVOID that, SOCKET sock, int counter);

	int authorize(SOCKET currentSocket);
	string pars_command(char *cmd);

	void start();
};

void Server::start()
{
	init();
	header(); 
	return;
}

Server::Server()
{
	result = 0;
	counter = 0;
	buf_size = 0;
	recvBuf_size = 0;
	recvBuffer = NULL;
	addrResult = NULL;
	ListenSocket = INVALID_SOCKET;

	user[0].ID = NONE;
	user[0].home_path = "/god/home";
	user[0].login = "god";
	user[0].password = "just_god";
	user[0].type_user = GOD;
	user[0].group = PRIORITY;
};

Server::~Server()
{
	delete[] recvBuffer;
	//delete[] buffer;
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
	SOCKET tempSock;
	for (int i = 0; i < MAX_LOAD; i++)
	{
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
		cout << "\nClient " << ClientSocket[counter] << " connected\n";
		tempSock = ClientSocket[counter];
		th[counter] = thread(thread_starter, this, tempSock, counter);
		counter++;
	}

	for (int i = 0; i < MAX_LOAD; i++)
		th[i].join();
}

void Server::thread_starter(LPVOID that, SOCKET sock, int counter)
{
	return ((Server*)that)->Client_header(sock, counter);
}

inline int Server::find_user(string login)
{
	for (int i = 0; i < MAX_LOAD; i++)
	{
		if (login == user[i].login)
			return i;
	}
	return -1;
}

inline int Server::authorize(SOCKET currentSocket)
{
	string login, pass;

	//get login
	recv(currentSocket, (char*)&recvBuf_size, sizeof(int), NULL);
	char* recvBuffer = new char[recvBuf_size];
	result = recv(currentSocket, recvBuffer, recvBuf_size, NULL);
	if (result == SOCKET_ERROR)
	{
		cout << "getting failed!" << endl;
		closesocket(currentSocket);
		freeaddrinfo(addrResult);
		WSACleanup();
		system("pause");
		exit(1);
	}
	login = recvBuffer;
	//--------------------
	cout << "\nReceived => " << recvBuffer << " :: " << currentSocket << endl;
	//-------------------
	delete[] recvBuffer;


	//get hash pass
	recv(currentSocket, (char*)&recvBuf_size, sizeof(int), NULL);
	recvBuffer = new char[recvBuf_size];
	result = recv(currentSocket, recvBuffer, recvBuf_size, NULL);
	if (result == SOCKET_ERROR)
	{
		cout << "getting failed!" << endl;
		closesocket(currentSocket);
		freeaddrinfo(addrResult);
		WSACleanup();
		system("pause");
		exit(1);
	}
	pass = recvBuffer;

	//--------------------
	cout << "\nReceived => " << recvBuffer << " :: " << currentSocket << endl;
	//-------------------
	int ind = find_user(login);
	if (ind == -1)
	{
		send_data(currentSocket, "ERROR! Invalid login! Try again.");
		delete[] recvBuffer;
		login.clear();
		pass.clear();
		result = SOCKET_ERROR;
		ind = -1;
		authorize(currentSocket);
	}
	else {
		cout << "\nfinded with index " << ind << endl;
	}

	if (user[ind].password == pass)
	{
		send_data(currentSocket, "successfully");
		user[ind].auth_token = true;
		user[ind].cur_sock = currentSocket;
		user[ind].online = true;
		user[ind].type_user = USER;
		return 1;
	}
	else
	{
		send_data(currentSocket, "ERROR! Wrong password! Try again.");
		delete[] recvBuffer;
		login.clear();
		pass.clear();
		result = SOCKET_ERROR;
		ind = -1;
		authorize(currentSocket);
	}
}

inline string Server::pars_command(char *cmd)
{
	return 0;
}

void Server::Client_header(SOCKET &currentSocket, int ind)
{
	string cmd;
	//связать структуру юзера с потоком
	authorize(currentSocket);


	do {
		recv(currentSocket, (char*)&recvBuf_size, sizeof(int), NULL);
		char* recvBuffer = new char[recvBuf_size];
		result = recv(currentSocket, recvBuffer, recvBuf_size, NULL);
			
		if (result == SOCKET_ERROR)
		{
			cout << "getting failed!" << endl;
			closesocket(ClientSocket[ind]);
			freeaddrinfo(addrResult);
			WSACleanup();
			system("pause");
			std::exit(1);
		}

		cout << recvBuffer << " :: " << currentSocket << endl;
		//============================================================================
		

		//cmd = pars_command(recvBuffer);
		cmd = "NONE";
		//pars command
		// autorize client
		// pars command
		// send data
		//

		send_data(currentSocket, cmd);
		delete[] recvBuffer;
	} while (true);
}

inline int Server::get_access()
{
	return 0;
}

inline int Server::set_access()
{
	return 0;
}

inline int Server::set_user()
{

	return 0;
}

inline int Server::get_user()
{
	return 0;
}

int Server::send_data(SOCKET &currentSocket, string cmd)
{
	cout << "\tSENDING";

	// -> отправляем размер буфера
	// -> отправляем сам буфер
	buf_size = cmd.size();
	cmd[buf_size] = '\0';
	buf_size++;
	send(currentSocket, (char*)&buf_size, sizeof(int), NULL);
	result = send(currentSocket, cmd.c_str(), buf_size, NULL);
	if (result == SOCKET_ERROR)
	{
		cout << "Send failed!" << endl;
		closesocket(ClientSocket[0]);
		freeaddrinfo(addrResult);
		WSACleanup();
		system("pause");
		exit(1);
	}
	else {
		cout << endl << currentSocket << " :: successfully\n";
	}
}