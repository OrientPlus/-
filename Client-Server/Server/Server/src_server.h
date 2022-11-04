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
#include <stack>
#include <thread>
#include <fstream>
#include <filesystem>
#include <map>

#include <openssl/sha.h>


//user levels
#define USER "user"
#define ADMIN "admin"
#define GOD "god"
#define WORM "worm"
//user groups
#define NONE "none"
#define PRIORITY "priority"

#define MAX_LOAD 5
#define CMD_COUNT 24;

#define MATRIX_LAW_PATH "LawMatrix.txt"
#define USERS_DATA_PATH "usersData.txt"


using namespace std;
namespace fs = std::filesystem;

class Server
{
private:
	//служебные стркутуры
	WSADATA wsaData;
	ADDRINFO hints;
	ADDRINFO* addrResult;
	SOCKET ClientSocket[MAX_LOAD] = { INVALID_SOCKET }, ListenSocket;
	thread th[MAX_LOAD];


	//матрица прав пользователей / (login + path) -> access
	map<pair<string, string>, int> LMatrix;

	//структура описания пользователя
	typedef struct USERS {
		string type_user = USER,
			ID = NONE,
			login = NONE,
			password = NONE,
			home_path = NONE,
			current_path = NONE;
		vector<string> group;
		bool auth_token = false,
			reserv_token = false;
		int cur_sock;
	};
	USERS user[MAX_LOAD];

	//стек свободных индексов для потоков
	stack<int> free_index;

	//вспомогательные переменные
	bool save_matrix_flag = false;
	int result, counter, buf_size, recvBuf_size,
		users_count =0;
	char *recvBuffer;
	string  _buffer, command, cmd_buffer,
		help_cmd;

	string supported_commands[24] = { "auth", "rr", "chmod", "help", "open", "close", "crF", "del",
		"crD", "cd", "crGr", "delGr", "addUnG", "delUnG", "write", "read", "ls", "logout",
	"crUser", "delUser", "chPUser", "chLUser", "listUser", "reg"};


	//===================================команды поддерживаемые сервером для клиента===================================
	int authorize(SOCKET currentSocket);
	int reg(SOCKET& currentSocket);
	int chmod(SOCKET &currentSocket);
	int help(SOCKET& currentSocket);
	int open(SOCKET& currentSocket);
	int close(SOCKET& currentSocket);
	int createFile(SOCKET& currentSocket);
	int deleteFD(SOCKET& currentSocket);
	int createDirectory(SOCKET& currentSocket);
	int cd(SOCKET& currentSocket);
	int createGroup(SOCKET& currentSocket);
	int deleteGroup(SOCKET& currentSocket);
	int addUserInGroup(SOCKET& currentSocket);
	int deleteUserInGroup(SOCKET& currentSocket);
	int write(SOCKET& currentSocket);
	int read(SOCKET& currentSocket);
	int ls(SOCKET& currentSocket);
	int logout(SOCKET& currentSocket, int ind);
	int createUser(SOCKET& currentSocket);
	int deleteUser(SOCKET& currentSocket);
	int changePUser(SOCKET& currentSocket);
	int changeLUser(SOCKET& currentSocket);
	int listUser(SOCKET& currentSocket);

	//===================================служебные функции сервера================================
	//получение и парсинг команд и опций 
	int get_command(SOCKET currentSocket, int ind);
	int pars_command(SOCKET& currentSocket, int th_id);
	string get_opt(int n, int opt_count);
	int send_data(SOCKET& currentSocket);
	int send_data(SOCKET& currentSocket, string cmd);
	
	//функции работы с матрицей прав
	int saveMatrixLaw();
	int addUserInMatrixLaw(string login, string group);
	int loadMatrixLaw();

	int get_level_access(int ind, string path);
	void set_level_access(int ind, string path, int access);

	//сохранение и загрузка структуры данных пользователей
	int load_users_data();
	int save_users_data();
	
	//авторизация и закрытие клиента 
	int close_client(SOCKET& currentSocket, int thread_ind, int client_ind);
	bool logged_in(SOCKET& currentSocket);

	//инициализация и структура сервера 
	void init();
	void header();
	void Client_header(SOCKET& currentSocket, int ind);
	void static thread_starter(LPVOID that, SOCKET sock, int counter);

	//функции работы с полем пользователя
	int find_user(string &login);
	int find_user(int curSock);
	int set_user();
	int get_user();

public:
	//функция запуска сервера
	void start();

	Server();
	~Server();
};
