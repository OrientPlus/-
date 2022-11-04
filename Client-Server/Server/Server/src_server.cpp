#include "src_server.h"

void Server::start()
{
	init();
	header();
	return;
}

int Server::load_users_data()
{
	//шифрование файла ??? OpenSSL ???
	ifstream file;

	file.open(USERS_DATA_PATH);
	if (file.bad())
	{
		cout << "\nUnable to open the file::usersData.txt\n";
		return -1;
	}

	string temp_gr;
	for (int i = 1; i < MAX_LOAD; i++) 
	{
		user[i].auth_token = false;
		file >> user[i].current_path;
		file >> temp_gr;
		while (temp_gr != "\\")
		{
			user[i].group.push_back(temp_gr);
			file >> temp_gr;
		}
		file >> user[i].home_path;
		file >> user[i].login;
		if (user[i].login != "none")
			users_count++;
		file >> user[i].password;
		file >> user[i].reserv_token;
		file >> user[i].type_user;
	}
	file.close();

	return 0;
}

int Server::save_users_data()
{
	ofstream file;
	fstream uInfo;

	uInfo.open(USERS_DATA_PATH);
	if (uInfo.bad())
	{
		cout << "\n\t\t\tUnable to open the file::usersData.txt  A new file will be created!\n";
		uInfo.close();

		file.open(USERS_DATA_PATH);
	}
	for (int i = 1; i < MAX_LOAD; i++)
	{
		file << user[i].current_path << endl;

		vector<string>::iterator it;
		it = user[i].group.begin();
		while (it != user[i].group.end())
		{
			file << it->c_str() << endl;
			it++;
		}
		file << "\\" << endl;
		file << user[i].home_path << endl;
		file << user[i].login << endl;
		file << user[i].password << endl;
		file << user[i].reserv_token << endl;
		file << user[i].type_user << endl;
	}
	file.close();
	return 0;
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

	//инициализируем СУПЕРпользователя
	user[0].ID = NONE;
	user[0].home_path = "/god/home";
	user[0].login = "god";
	user[0].password = "just_god";
	user[0].type_user = GOD;
	user[0].group.push_back(PRIORITY);
	user[0].reserv_token = true;
	user[0].current_path = user[0].home_path;

	//заполняем структуру информации о пользователях
	load_users_data();

	//загружаем матрицу прав из файла (заполнение ассоциативного массима map)
	if (!loadMatrixLaw())
	{
		cout << "\n\t\t\tError loading the access matrix file!\n";
		system("pause");
		//КОРРЕКТНОЕ ЗАКРЫТИЕ СЕРВЕРА?
		exit(-1);
	}

	//определяем свободные индексы для массива потоков
	for (int i = MAX_LOAD - 1; i >= 0; i--)
		free_index.push(i);

	help_cmd = "Supported commands:\
\n\t* help - displays help about commands. Parameters: help [command] \nIf the option is not set, it displays information about all commands\
\n\t* reg - Registers a new client on the server with minimal rights. \nParameters: reg [login] [password]\
\n\t* auth - authorization command. Parameters: auth [login] [password]\
\n\t* rr - Returns the user's rights to a specific file. \nParameters: rr [filename]\
\n\t* write - the command to write to a file. Parameters: write [file_name] [text]\
\n\t* read - a command to read from a file. Parameters: write [file_name]\
\n\t* ls - directory listing command. Parameters: ls [dir]; \nif no parameters are specified, the current directory is listed.\
\n\t* logout - the command to disconnect from the server. Has no parameters\
\n\t* chmod - command to set file/directory permissions. \nParameters: chmod [file name] [for all] [for other groups] [for user group]\nExample: chmod user/home/file1.txt -r- -r- r-x\
\n\t* crF - creates a file with the specified name. Parameters: crF [file_name]\
\n\t* crD - creates a directory with the specified name. Parameters: crD [path/dir_name]\nIf the path is not specified, creates in the current directory.\
\n\t* del - deletes the file or directory with the specified name, if the user has the rights to do so. \nParameters: rf [file_name/directory]\
\n\t* cd - go to the specified directory. Parameters: cd [path]\
\n\t* crGr - create a group with the specified access rights. \nThe user who created this group is included in it by default. \nParameters: crGr [gr_name] [law for other user] [users who included in group]\
\n\t* delGr - deletes the specified group if the user has the rights to do so. \nParameters: delGr [gr_name]\
\n\t* addUnG - add user in group\
\n\t* delUnG - del user in group\
\nCOMMANDS FOR ADMIN ONLY:\
\n\t* crUser - the user creation command. Parameters: crUser [user_name] [user_password]\
\n\t* delUser - the user deletion command. If a user with such a login is found, \nit deletes it, as well as its files and directories. \nParameters: delUser [user_login]\
\n\t* chLUser - the command for setting user rights. Determines the user's access level on the server. \nParameters: chLUser [option]. \nPossible access levels: ADMIN, USER, WORM\
\n\t* chPUser - ???\
\n\t* listUser - get information about all users. Has no parameters.";
};

Server::~Server()
{
	delete[] recvBuffer;
	//delete[] buffer;
}

int Server::close_client(SOCKET& currentSocket, int thread_ind, int client_ind) 
{
	user[client_ind].auth_token = false;
	save_users_data();
	shutdown(currentSocket, NULL);
	closesocket(currentSocket);
	//th[thread_ind].~thread();
	return 0;
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

	while(free_index.size() != 0)
	{
		counter = free_index.top();
		free_index.pop();

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
	}

	for (int i = 0; i < MAX_LOAD; i++)
		th[i].join();
}

void Server::thread_starter(LPVOID that, SOCKET sock, int counter)
{
	return ((Server*)that)->Client_header(sock, counter);
}

int Server::find_user(string &login)
{
	for (int i = 0; i < MAX_LOAD; i++)
	{
		if (login == user[i].login)
			return i;
	}
	return -1;
}

int Server::find_user(int curSock)
{
	for (int i = 0; i < MAX_LOAD; i++)
	{
		if (user[i].cur_sock == curSock)
			return i;
	}
	return -1;
}

inline int Server::authorize(SOCKET currentSocket)
{
	string login, pass;
	int ind;

	login = get_opt(0, 2);
	pass = get_opt(1, 2);

	if (login == "none")
	{
		send_data(currentSocket, "ERROR! Invalid login!\n");
		return -1;
	}

	if (login == "error")
	{
		send_data(currentSocket, "ERROR! Invalid command arguments!\n");
		return -1;
	}
	
	ind = find_user(login);
	if (user[ind].auth_token == true)
	{
		send_data(currentSocket, "This user is already logged in to another thread!\n");
		return -1;
	}
	if (ind == -1)
	{
		send_data(currentSocket, "ERROR! Invalid login!\n");
		return -1;
	}
	else {
		if (user[ind].password == pass)
		{
			user[ind].current_path = user[ind].home_path;
			user[ind].auth_token = true;
			user[ind].cur_sock = currentSocket;
			send_data(currentSocket, user[ind].home_path);
			return 0;
		}
		else {
			send_data(currentSocket, "ERROR! Invalid password!\n");
			return -1;
		}
	}
}

inline int Server::pars_command(SOCKET& currentSocket, int th_id)
{
	string help_buf;
	int switch_on, ret = -1;
	command.clear();
	command.reserve(10);
	help_buf.reserve(20);
	//вырезаем из буфера команду и оставляем только доп. данные
	for (int i = 0; i < cmd_buffer.size(); i++)
	{
		if (cmd_buffer[i] == ' ')
		{
			i++;
			for (i; i < cmd_buffer.size(); i++)
			{
				help_buf += cmd_buffer[i];
			}
			//help_buf[i] = '\0';
			cmd_buffer.clear();
			cmd_buffer = help_buf;
			break;
		}
		else {
			command += cmd_buffer[i];
		}
	}
	if (help_buf.size() == 0)
		cmd_buffer.clear();

	//ищем команду в БД команд
	for (int i = 0; i < 24; i++)
	{
		if (command == supported_commands[i])
		{
			switch_on = i;
			break;
		}
		else if (i == 23)
		{
			command.clear();
			cmd_buffer.clear();
			return -10;
		}
	}

	switch (switch_on)
	{
	case 0:
		ret = authorize(currentSocket);
		return ret;
	case 1:
		ret = send_data(currentSocket, "This command is not supported in the current version of the server! I'm working on it :)\n");
		return ret;
	case 2:
		if (logged_in(currentSocket))
			ret = chmod(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 3:
		ret = help(currentSocket);
		return ret;
	case 4:
		send_data(currentSocket, "This command is not supported in the current version of the server! I'm working on it :)\n");
		return 0;
	case 5:
		send_data(currentSocket, "This command is not supported in the current version of the server! I'm working on it :)\n");
		return 0;
	case 6:
		if (logged_in(currentSocket))
			ret = createFile(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 7:
		if (logged_in(currentSocket))
			ret = deleteFD(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 8:
		if (logged_in(currentSocket))
			ret = createDirectory(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 9:
		if (logged_in(currentSocket))
			ret = cd(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 10:
		if (logged_in(currentSocket))
			ret = createGroup(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 11:
		if (logged_in(currentSocket))
			ret = deleteGroup(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 12:
		if (logged_in(currentSocket))
			ret = addUserInGroup(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 13:
		if (logged_in(currentSocket))
			ret = deleteUserInGroup(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 14:
		if (logged_in(currentSocket))
			ret = write(currentSocket); 
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 15:
		if (logged_in(currentSocket))
			ret = read(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 16:
		if (logged_in(currentSocket))
			ret = ls(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 17:
		if (logged_in(currentSocket))
			ret = logout(currentSocket, th_id);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 18:
		if (logged_in(currentSocket))
			ret = createUser(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 19:
		if (logged_in(currentSocket))
			ret = deleteUser(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 20:
		if (logged_in(currentSocket))
			ret = changePUser(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 21:
		if (logged_in(currentSocket))
			ret = changeLUser(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 22:
		if (logged_in(currentSocket))
			ret = listUser(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 23:
		ret = reg(currentSocket);
		return ret;
	default:
		return -10;
	}
}

int Server::get_command(SOCKET currentSocket, int ind)
{
	recv(currentSocket, (char*)&recvBuf_size, sizeof(int), NULL);
	recvBuffer = new char[recvBuf_size];
	result = recv(currentSocket, recvBuffer, recvBuf_size, NULL);
	if (result == SOCKET_ERROR)
	{
		cout << "getting failed!\n" << endl;
		closesocket(ClientSocket[ind]);
		freeaddrinfo(addrResult);
		WSACleanup();
		system("pause");
		std::exit(1);
	}
	else if (result < recvBuf_size)
	{
		cout << "\n\t\tWAITING ALL DATA\n\n";
		system("pause");
	}

	cout << recvBuffer << " :: " << currentSocket << endl;
	cmd_buffer = recvBuffer;
	delete[] recvBuffer;
	//============================================================================

}

void Server::Client_header(SOCKET& currentSocket, int ind)
{
	int ex_flag = 0;
	do {
		get_command(currentSocket, ind);
		ex_flag = pars_command(currentSocket, ind);
		if (ex_flag == -10)
		{
			int _ind = find_user(currentSocket);
			send_data(currentSocket, "Invalid command!\n" + user[_ind].current_path);
		}
		else if (ex_flag == 1234)
			break;
	} while (true);
}

inline int Server::get_level_access(int ind, string path)
{
	pair<string, string> key = make_pair<string, string>(user[ind].login.c_str(), path.c_str());

	if (LMatrix.find(key) != LMatrix.end())
		return LMatrix.find(key)->second;
	for (int i = 0; i < user[ind].group.size(); i++)
	{
		if (LMatrix.find(make_pair<string, string>(user[ind].group[i].c_str(), path.c_str())) != LMatrix.end())
			return LMatrix.find(make_pair<string, string>(user[ind].group[i].c_str(), path.c_str()))->second;
	}

	return -1;
}

inline void Server::set_level_access(int ind, string path, int access)
{
	//pair<string, string> key = make_pair<string, string>(user[ind].login.c_str(), path.c_str());
	LMatrix.insert(make_pair<pair<string, string>, int>(make_pair<string, string>(user[ind].login.c_str(), path.c_str()), static_cast<int>(access)));

	if (access / 10 % 10 != 0)
	{
		for (int i = 0; i < user[ind].group.size(); i++)
			LMatrix.insert(make_pair<pair<string, string>, int>(make_pair<string, string>(user[ind].group[i].c_str(), path.c_str()), static_cast<int>(access))); //???????
	}
	if (access % 100 != 0)
		LMatrix.insert(make_pair<pair<string, string>, int>(make_pair<string, string>("OTHER GROUP", path.c_str()), static_cast<int>(access)));

	fstream out_file(MATRIX_LAW_PATH);

	map<pair<string, string>, int>::iterator it;
	if (!out_file.is_open())
		out_file.open(MATRIX_LAW_PATH);

	for (it = LMatrix.begin(); it != LMatrix.end(); it++)
		out_file << it->first.first << it->first.second << it->second << endl;

	out_file.close();
}

inline int Server::set_user()
{

	return 0;
}

inline int Server::get_user()
{
	return 0;
}

inline int Server::send_data(SOCKET& currentSocket, string cmd)
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

inline int Server::send_data(SOCKET& currentSocket)
{
	cout << "\tSENDING";

	// -> отправляем размер буфера
	// -> отправляем сам буфер
	buf_size = command.size();
	command[buf_size] = '\0';
	buf_size++;
	send(currentSocket, (char*)&buf_size, sizeof(int), NULL);
	result = send(currentSocket, command.c_str(), buf_size, NULL);
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

int Server::reg(SOCKET& currentSocket)
{
	string log, pass;
	log.reserve(10); 
	pass.reserve(20);

	//получаем из буффера команды логин и пароль
	log = get_opt(0, 2);
	pass = get_opt(1, 2);
	//если опций не две, вернем юзеру ошибку
	if (log == "error")
	{
		send_data(currentSocket, "ERROR! Invalid command arguments!\n");
		return -1;
	}
	else if (log == "none")
	{
		send_data(currentSocket, "ERROR! Invalid login!\n");
		return -1;
	}

	//проверяем логин и пароль на пустоту
	if (log.size() < 2)
	{
		send_data(currentSocket, "ERROR! Invalid options in the command!\n");
		return -1;
	}
	if (pass.size() < 2)
	{
		send_data(currentSocket, "ERROR! Invalid options in the command!\n");
		return -1;
	}

	//проверяем логин и пароль на недопустимые символы
	for (int i = 0; i < log.size(); i++)
	{
		if (log[i] == '&' || log[i] == '$' || log[i] == '~' || log[i] == '`' || log[i] == '"' || log[i] == '%' || log[i] == '(' || log[i] == ')' )
		{
			send_data(currentSocket, "ERROR! Invalid options in command! Invalid character in the login\n");
			return -1;
		}
	}
	for (int i = 0; i < pass.size(); i++)
	{
		if (pass[i] == '&' || pass[i] == '$' || pass[i] == '~' || pass[i] == '`' || pass[i] == '"' || pass[i] == '%' || pass[i] == '(' || pass[i] == ')')
		{
			send_data(currentSocket, "ERROR! Invalid options in command! Invalid character in the password\n");
			return -1;
		}
	}

	//проверяем не занят ли этот логин
	if (find_user(log) != -1)
	{
		send_data(currentSocket, "ERROR! A user with this username already exists!\n");
		return -1;
	}


	//инициализируем структуру нового юзера и добавляем его в матрицу прав
	for (int i = 0; i < MAX_LOAD; i++)
	{
		if (user[i].reserv_token == false)
		{
			user[i].reserv_token = true;
			user[i].auth_token = true;
			user[i].current_path = log + "\\home";
			user[i].cur_sock = currentSocket;
			user[i].home_path = log + "\\home";
			user[i].login = log;
			user[i].password = pass;
			user[i].type_user = WORM;
			user[i].group = NONE;
			addUserInMatrixLaw(log, "none");
			send_data(currentSocket, user[i].current_path);
			fs::create_directories("C:\\Users\\gutro\\Desktop\\server_root\\" + user[i].current_path);
			break;
		}
	}

	//создаем домашнюю директорию для пользователя

	return 0;
}

int Server::chmod(SOCKET& currentSocket)
{
	return 0;
}

int Server::help(SOCKET& currentSocket)
{
	send_data(currentSocket, help_cmd.c_str());
	return 0;
}

int Server::createFile(SOCKET& currentSocket)
{

	return 0;
}

int Server::deleteFD(SOCKET& currentSocket)
{
	return 0;
}

int Server::createDirectory(SOCKET& currentSocket)
{
	// ++ проверка на существование директории с таким именем
	// ++ проверка на допустимые символы в имени директории 
	// проверка прав на создание по такому пути
	int ind = find_user(currentSocket);
	string dir_name, opt,
		path = "C:\\Users\\gutro\\Desktop\\server_root\\";


	dir_name.reserve(15);
	opt = get_opt(0, 1);
	for (int i = 0; i<opt.size(); i++)
	{
		if (opt[i] == 34 || opt[i] == 42 || opt[i] == 47 || opt[i] == 58 || opt[i] == 60 || opt[i] == 62 || opt[i] == 63  || opt[i] == 124)
		{
			send_data(currentSocket, "ERROR! Invalid symbol in the directory name!\n" + user[ind].current_path);
			return 0;
		}
		dir_name += opt[i];
	}
	if (dir_name[0] == '\\')
		path += dir_name;
	else
		path += user[ind].current_path + dir_name;
	
	//существование директории с таким именем
	if (fs::exists(path))
	{
		send_data(currentSocket, "ERROR! A directory with this name already exists!\n" + user[ind].current_path);
		return 0;
	}
	//наличие прав на создание в этой директории
	if (!get_access(ind, path))
	{
		send_data(currentSocket, "ERROR! You don't have access rights!\n" + user[ind].current_path);
		return 0;
	}
	if (fs::create_directories(path) == -1)
	{
		send_data(currentSocket, "ERROR! Created directory end with failed!\n" + user[ind].current_path);
		return 0;
	}
	else
		user[ind].current_path = user[ind].home_path + "\\" + dir_name;
	send_data(currentSocket, user[ind].current_path.c_str());

	return 0;
}

int Server::cd(SOCKET& currentSocket)
{
	string opt;
	int ind = find_user(currentSocket);
	/*if (ind == -1)
	{
		send_data(currentSocket, "ERROR! Log in before using this command!\n" + user[ind].current_path);
		return -1;
	}
	if (!user[ind].auth_token)
	{
		send_data(currentSocket, "ERROR! Log in before using this command!\n" + user[ind].current_path);
		return -1;
	}*/
	if (cmd_buffer.size() == 0)
	{
		user[ind].current_path = user[ind].home_path;
		send_data(currentSocket, user[ind].current_path);
		return 0;
	}
	else {
		opt = get_opt(0, 1);
		//проверить существование такой директории
		//проверить доступ к директории в матрице прав
		user[ind].current_path += opt;
		send_data(currentSocket, user[ind].current_path);
	}
	return 0;
}

int Server::createGroup(SOCKET& currentSocket)
{
	return 0;
}

int Server::deleteGroup(SOCKET& currentSocket)
{
	return 0;
}

int Server::addUserInGroup(SOCKET& currentSocket)
{
	return 0;
}

int Server::deleteUserInGroup(SOCKET& currentSocket)
{
	return 0;
}

int Server::write(SOCKET& currentSocket)
{
	return 0;
}

int Server::read(SOCKET& currentSocket)
{
	return 0;
}

int Server::ls(SOCKET& currentSocket)
{
	return 0;
}

int Server::logout(SOCKET& currentSocket, int th_id)
{
	int cl_id = find_user(currentSocket);
	send_data(currentSocket, "disconnected");
	
	//закрываем клиентский сокет
	close_client(currentSocket, th_id, cl_id);

	//добавляем освободившийся индекс в стек
	free_index.push(th_id);
	return 1234;
}

int Server::createUser(SOCKET& currentSocket)
{
	return 0;
}

int Server::deleteUser(SOCKET& currentSocket)
{
	return 0;
}

int Server::changePUser(SOCKET& currentSocket)
{
	return 0;
}

int Server::changeLUser(SOCKET& currentSocket)
{
	return 0;
}

int Server::listUser(SOCKET& currentSocket)
{
	return 0;
}

int Server::loadMatrixLaw()
{
	fstream law;
	string key1, key2;
	bool value = false;
	law.open(MATRIX_LAW_PATH);

	if (!law.is_open())
	{
		cout << "\n\t\tERROR OPEN FILE 'LawMatrix.txt'!\n";
		return 0;
	}
	while (law.eof() != true)
	{
		law >> key1 >> key2 >> value;
		LMatrix.insert( make_pair<pair<string, string>, bool> (make_pair<string, string>(key1.c_str(), key2.c_str()), &value));
	}
	return 1;
}

int Server::addUserInMatrixLaw(string login, string group)
{
	return 0;
}

string Server::get_opt(int n, int opt_count)
{
	string opt;
	int numb_opt = n;

	for (int i = 0; i < cmd_buffer.size(); i++)
	{
		if (cmd_buffer[i] == ' ')
			opt_count--;
	}
	if (opt_count < 0)
		return "error";

	opt.reserve(20);
	for (int i = 0; i < cmd_buffer.size(); i++)
	{
		if (cmd_buffer[i] == ' ' && numb_opt == 0)
			break;
		if (cmd_buffer[i] == ' ' && numb_opt != 0) {
			numb_opt--;
			continue;
		}
		if (numb_opt == 0)
			opt += cmd_buffer[i];
	}
	opt[opt.size()] = '\0';
	return opt;
}

bool Server::logged_in(SOCKET& currentSocket)
{
	int ind = find_user(currentSocket);
	if (ind != -1)
	{
		if (user[ind].auth_token)
			return true;
		else
			return false;
	}
	else
		return false;
}