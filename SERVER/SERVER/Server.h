#pragma once

#define WIN32_LEAN_AND_MEAN
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib,"Crypt32.lib")
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



#define PRIORITY "priority"
#define OTHER_GROUP "other_group"

#define MAX_LOAD 5
#define CMD_COUNT 24;

#define MATRIX_LAW_PATH "LawMatrix.txt"
#define USERS_DATA_PATH "usersData.txt"
#define SERVER_ROOT_PATH "C:\\Users\\gutro\\Desktop\\server_root"

#define DIR_FL 1
#define FILE_FL 2

#define INV_SYMBOL_IN_PATH -1
#define EXIT -10
#define INV_CMD -30



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
	map<pair<string, string>, string> LMatrix;

	enum { USER, ADMIN, GOD };

	//структура описания пользователя
	class USERS {
	public:
		string login,
			password,
			home_path,
			current_path;
		vector<string> group;
		bool auth_token, del_tocken;
		int cur_sock, status, th_id;
	};


	USERS* user;
	USERS* reallocated(int size);
	size_t users_count;

	//стек свободных индексов для потоков
	stack<int> free_index;

	//вспомогательные переменные
	bool save_matrix_flag = false;
	int result, counter, buf_size, recvBuf_size,
		err_val1, err_val2;
	char* recvBuffer;
	string  _buffer, command, cmd_buffer,
		help_all_cmd, help_cmds[22];

	string supported_commands[22] = { "help", "reg", "auth", "rr", "write", "read",
		"ls", "logout", "chmod", "crF", "crD", "del", "cd", "crGr", "delGr", "addUnG",
	"delUnG", "crUser", "delUser", "chLUser", "chPUser", "listUser" };


	//===================================команды поддерживаемые сервером для клиента===================================
	int authorize(SOCKET currentSocket, int th_id);
	int reg(SOCKET& currentSocket, int initiator, int th_id);
	int chmod(SOCKET& currentSocket);
	int help(SOCKET& currentSocket);
	int rr(SOCKET& currentSocket);
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
	int logout(SOCKET& currentSocket, int th_id);
	int createUser(SOCKET& currentSocket, int th_id);
	int deleteUser(SOCKET& currentSocket, int th_id);
	int changePUser(SOCKET& currentSocket);
	int changeLUser(SOCKET& currentSocket);
	int listUser(SOCKET& currentSocket);

	//===================================служебные функции сервера================================
	//получение и парсинг команд и опций 
	int get_command(SOCKET currentSocket);
	int pars_command(SOCKET& currentSocket, int th_id);
	vector<string> get_opt(bool AUTH_FL);
	int send_data(SOCKET& currentSocket);
	int send_data(SOCKET& currentSocket, string cmd);

	int check_path(string& path, int fl);
	string get_owner(string path);

	//функции работы с матрицей прав
	void saveMatrixLaw();
	//int addUserInMatrixLaw(string login, string group);
	int loadMatrixLaw();

	string get_level_access(int ind, string path, bool INFO_FL);
	void set_level_access(int ind, string path, string& access);
	void change_level_access(int ind, string path, string& access);
	void delete_level_access(int ind, string path);

	//сохранение и загрузка структуры данных пользователей
	int load_users_data();
	int save_users_data();

	//авторизация и закрытие клиента 
	int close_client(SOCKET& currentSocket, int thread_ind, int client_ind);
	bool logged_in(SOCKET& currentSocket);
	bool logged_in(int currentSocket);

	//инициализация и структура сервера 
	void init();
	void header();
	void Client_header(SOCKET& currentSocket, int ind);
	void static thread_starter(LPVOID that, SOCKET sock, int counter);

	//функции работы с полем пользователя
	int find_user(string& login);
	int find_user(int curSock);

public:
	//функция запуска сервера
	void start();

	Server();
	~Server();
};
