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
//#define USER "user"
//#define ADMIN "admin"
//#define GOD "god"
//#define WORM "worm"
//user groups
#define PRIORITY "priority"

#define MAX_LOAD 5
#define CMD_COUNT 24;

#define MATRIX_LAW_PATH "LawMatrix.txt"
#define USERS_DATA_PATH "usersData.txt"
#define SERVER_ROOT_PATH "C:\\Users\\gutro\\Desktop\\server_root"

#define OTHER_GROUP "other_group"
#define DIR_FL 1
#define FILE_FL 2

#define ERROR_STR "error"
#define INV_SYMBOL_IN_PATH -1



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

	enum { GOD, ADMIN, USER };

	//структура описания пользователя
	class USERS {
	public:
		string login,
			password,
			home_path,
			current_path;
		vector<string> group;
		bool auth_token;
		int cur_sock, status;
	};

	
	USERS *user;
	USERS* reallocated(size_t size);
	size_t users_count;

	//стек свободных индексов для потоков
	stack<int> free_index;

	//вспомогательные переменные
	bool save_matrix_flag = false;
	int result, counter, buf_size, recvBuf_size;
	char *recvBuffer;
	string  _buffer, command, cmd_buffer,
		help_all_cmd, help_cmds[22];

	string supported_commands[22] = { "help", "reg", "auth", "rr", "write", "read",
		"ls", "logout", "chmod", "crF", "crD", "del", "cd", "crGr", "delGr", "addUnG",
	"delUnG", "crUser", "delUser", "chLUser", "chPUser", "listUser" };


	//===================================команды поддерживаемые сервером для клиента===================================
	int authorize(SOCKET currentSocket);
	int reg(SOCKET& currentSocket, int initiator);
	int chmod(SOCKET &currentSocket);
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

	int check_path(string& path, int fl);
	string get_owner(string path);
	
	//функции работы с матрицей прав
	void saveMatrixLaw();
	int addUserInMatrixLaw(string login, string group);
	int loadMatrixLaw();

	string get_level_access(int ind, string path, bool INFO_FL);
	void set_level_access(int ind, string path, string &access);
	void change_level_access(int ind, string path, string& access);
	void delete_level_access(int ind, string path);

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
