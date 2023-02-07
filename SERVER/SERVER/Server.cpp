#include "Server.h"

//________________________________________________
//      инициализация и запуск сервера           |
//------------------------------------------------

Server::Server()
{
	result = 0;
	counter = 0;
	buf_size = 0;
	recvBuf_size = 0;
	recvBuffer = NULL;
	addrResult = NULL;
	ListenSocket = INVALID_SOCKET;

	fstream file(USERS_DATA_PATH);
	file >> users_count;
	file.close();


	try
	{
		user = new USERS[users_count];
	}
	catch (exception ex)
	{
		cout << "\nCant allocate memory! :: " << ex.what() << endl;
	}

	//инициализируем СУПЕРпользователя
	user[0].login = "god";
	user[0].password = "581282523510841531571641049818249153022108171301450109431851121472104614982011196952499919422817921216021314202181233516115106211711510464183113204226709711231178";
	user[0].home_path = "\\god\\home";
	user[0].current_path = "\\god\\home";
	user[0].group.push_back("god");
	user[0].group.push_back(PRIORITY);
	user[0].auth_token = false;
	user[0].status = GOD;
	user[0].del_tocken = false;
	user[0].current_mark = _GOD;
	user[0].def_mark = _GOD;

	//инициализируем логгирующего пользователя
	init_log_user();

	//загружаем последний журнал логов, если он есть
	fstream in;
	if(fs::exists(LOG_JOURNAL_PATH))
	{
		in.open(LOG_JOURNAL_PATH, ios::in);
		in >> journal_size;
		string tmp;
		for(int i=0; i<journal_size; i++)
		{
			in >> tmp;
			journal.push_back(tmp);
			tmp.clear();
		}
	}

	//заполняем структуру информации о пользователях
	load_users_data();

	//инициализируем контейнер отлеживаемых субъектов
	journal_size = 5;
	in_work = false;
	for (int i = 0; i < users_count; i++)
	{
		if (user[i].login == "logger")
			continue;
		tracked_subj.push_back(pair<string, string>(user[i].login, "00000"));
		for (int j = 1; j < user[i].group.size(); j++)
			tracked_subj.push_back(pair<string, string>(user[i].group[j], "00000"));
	}
	load_attr();

	//определяем матрицу прав из файла (заполнение ассоциативного массима map)
	if (!loadMatrixLaw())
	{
		cout << "\n\t\aError loading the access matrix file!\n";
		system("pause");
		exit(-1);
	}

	//инициализируем и определяем структуру мандатной системы
	if (ms.load_marks() == -1)
	{
		cout << "\n\t\aThe file with the data of the mandatory system could not be uploaded!\n";
		system("pause");
		exit(-1);
	}


	//определяем свободные индексы для массива потоков
	for (int i = MAX_LOAD - 1; i >= 0; i--)
		free_index.push(i);
	help_cmds[0] = "\n\t* help - displays help about commands. Parameters: help [command] \nIf the option is not set, it displays information about all commands";
	help_cmds[1] = "\n\t* reg - Registers a new client on the server with minimal rights. \nParameters: reg [login] [hash of password]. \nIf the parameters are not passed, the login and password hash are passed separately immediately after the command is called.";
	help_cmds[2] = "\n\t* auth - authorization command. Parameters: auth [login] [hash of password]. \nIf the parameters are not passed, the login and password hash are passed separately immediately after the command is called.";
	help_cmds[3] = "\n\t* rr - Returns the user's rights to a specific file. \nParameters: rr [filename]";
	help_cmds[4] = "\n\t* write - the command to write to a file. Parameters: write [file_name] [text]";
	help_cmds[5] = "\n\t* read - a command to read from a file. Parameters: read [file_name]";
	help_cmds[6] = "\n\t* ls - directory listing command. Parameters: ls [dir]; \nif no parameters are specified, the current directory is listed.";
	help_cmds[7] = "\n\t* logout - the command to disconnect from the server. Has no parameters";
	help_cmds[8] = "\n\t* chmod - command to set file/directory permissions. \nParameters: chmod [file name] [access value]\nExample: chmod user\\home\\file1.txt 777";
	help_cmds[9] = "\n\t* crF - creates a file with the specified name. Parameters: crF [file_name] [access value]";
	help_cmds[10] = "\n\t* crD - creates a directory with the specified name. Parameters: crD [path/dir_name] [access value]\nIf the path is not specified, creates in the current directory.";
	help_cmds[11] = "\n\t* del - deletes the file or directory with the specified name, if the user has the rights to do so. \nParameters: del [file_name/directory]";
	help_cmds[12] = "\n\t* cd - go to the specified directory. Parameters: cd [path]. \nIf no parameters are specified, it goes to the user's home directory.";
	help_cmds[13] = "\n\t* crGr - create a group with the specified access rights. \nThe user who created this group is included in it by default. \nParameters: crGr [gr_name] [access value]";
	help_cmds[14] = "\n\t* delGr - deletes the specified group if the user has the rights to do so. \nParameters: delGr [gr_name]";
	help_cmds[15] = "\n\t* addUnG - add user in group";
	help_cmds[16] = "\n\t* delUnG - del user in group";
	help_cmds[17] = "\n\t* crUser - the user creation command.";
	help_cmds[18] = "\n\t* delUser - the user deletion command. If a user with such a login is found, \nit deletes it, as well as its files and directories. \nParameters: delUser [user_login]";
	help_cmds[19] = "\n\t* chLUser - the command for setting user rights. Determines the user's access level on the server. \nParameters: chLUser [option]. \nPossible access levels: ADMIN, USER, WORM";
	help_cmds[20] = "\n\t* chPUser - Changes the password by the specified username.";
	help_cmds[21] = "\n\t* listUser - get information about all users. Has no parameters.";

	help_all_cmd = "Supported commands:\
\n\t* help - displays help about commands. Parameters: help [command] \nIf the option is not set, it displays information about all commands\
\n\t* reg - Registers a new client on the server with minimal rights. \nParameters: reg [login] [password]\
\n\t* auth - authorization command. Parameters: auth [login] [hash of password]\
\n\t* rr - Returns the user's rights to a specific file. \nParameters: rr [filename]\
\n\t* write - the command to write to a file. Parameters: write [file_name] [text]\
\n\t* read - a command to read from a file. Parameters: read [file_name]\
\n\t* ls - directory listing command. Parameters: ls [dir]; \nif no parameters are specified, the current directory is listed.\
\n\t* logout - the command to disconnect from the server. Has no parameters\
\n\t* chmod - command to set file/directory permissions. \nParameters: chmod [file name] [access value]\nExample: chmod user\\home\\file1.txt 777\
\n\t* crF - creates a file with the specified name. Parameters: crF [file_name] [access value]\
\n\t* crD - creates a directory with the specified name. Parameters: crD [path/dir_name] [access value]\nIf the path is not specified, creates in the current directory.\
\n\t* del - deletes the file or directory with the specified name, if the user has the rights to do so. \nParameters: del [file_name/directory]\
\n\t* cd - go to the specified directory. Parameters: cd [path]. \nIf no parameters are specified, it goes to the user's home directory.\
\n\t* crGr - create a group with the specified access rights. \nThe user who created this group is included in it by default. \nParameters: crGr [gr_name] [access value]\
\n\t* delGr - deletes the specified group if the user has the rights to do so. \nParameters: delGr [gr_name]\
\n\t* addUnG - add user in group\
\n\t* delUnG - del user in group\
\nCOMMANDS FOR ADMIN ONLY:\
\n\t* crUser - the user creation command.\
\n\t* delUser - the user deletion command. If a user with such a login is found, \nit deletes it, as well as its files and directories. \nParameters: delUser [user_login]\
\n\t* chLUser - the command for setting user rights. Determines the user's access level on the server. \nParameters: chLUser [option]. \nPossible access levels: ADMIN, USER, WORM\
\n\t* chPUser - Changes the password by the specified username.\
\n\t* listUser - get information about all users. Has no parameters.";
};

Server::~Server()
{
	delete[] recvBuffer;
	//delete[] buffer;
}

void Server::start()
{
	init();
	header();
	return;
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
	cout << "\n#################################_Waiting for the first connection._#################################\n";

	while (free_index.size() != 0)
	{
		counter = free_index.top();
		free_index.pop();

		ClientSocket[counter] = accept(ListenSocket, NULL, NULL);
		if (ClientSocket[counter] == INVALID_SOCKET)
		{
			cout << "Accepted error!" << endl;
			shutdown(ClientSocket[counter], 2);
			closesocket(ClientSocket[counter]);
			int er = WSAGetLastError();
			cout << endl << "WSA ERROR :: " << er << " SOCKET:: " << ClientSocket[counter] << " THREAD:: " << counter << endl;
			WSACleanup();
			er = WSAGetLastError();
			cout << "WSA ERROR :: " << er << endl;
			freeaddrinfo(addrResult);
			system("pause");
			exit(1);
		}
		cout << "\n----------------------Client " << ClientSocket[counter] << " connected----------------------\n";

		th[counter] = thread(thread_starter, this, ClientSocket[counter], counter);
	}

	for (int i = 0; i < MAX_LOAD; i++)
		th[i].join();
}

void Server::thread_starter(LPVOID that, SOCKET sock, int counter)
{
	((Server*)that)->Client_header(sock, counter);
}

void Server::Client_header(SOCKET& currentSocket, int ind)
{
	int flag = 0;
	do {
		flag = get_command(currentSocket);
		if (flag == EXIT)
			break;
		flag = pars_command(currentSocket, ind);
		if (flag == INV_CMD)
		{
			int _ind = find_user(currentSocket);
			send_data(currentSocket, "Invalid command!\n" + user[_ind].current_path);
		}
		else if (flag == EXIT)
			break;

	} while (true);
}

int Server::close_client(SOCKET& currentSocket, int thread_ind, int client_ind)
{
	shutdown(currentSocket, 2);
	closesocket(currentSocket);

	return 0;
}

//________________________________________
//    функции обработки команд сервера   |
//----------------------------------------
int Server::authorize(SOCKET currentSocket, int th_id)
{
	struct auth_counter {
		int counter;
		SOCKET sock;
	};
	static vector<pair<int, SOCKET>> count = { {0, currentSocket} };
	vector<pair<int, SOCKET>>::iterator it;
	vector<pair<string, string>>::iterator s_it, s_it_g;

	string login, pass;
	int ind, mark;
	
	if (cmd_buffer.size() != 0)
	{
		vector<string> box_opt = get_opt(false);
		if (box_opt.size() != 3)
		{
			send_data(currentSocket, "ERROR! Invalid command options!\n");
			return -1;
		}
		login = box_opt[0];
		pass = box_opt[1];
		mark = to_integer<int>(static_cast<byte>(box_opt[2][0]));
		mark -= '0';
	}
	else {
		send_data(currentSocket, "LOGIN->");
		get_command(currentSocket);
		login = cmd_buffer;
		send_data(currentSocket, "PASS->");
		get_command(currentSocket);
		pass = cmd_buffer;
		send_data(currentSocket, "MARK->");
		get_command(currentSocket);
		mark = std::to_integer<int>(static_cast<byte>(cmd_buffer[0]));
		mark -= '0';

		int size = pass.size();
		unsigned char* _pass;
		unsigned char hash[64];
		_pass = new unsigned char[size + 1];
		for (int i = 0; i < pass.size(); i++)
			_pass[i] = pass[i];
		pass.clear();
		pass.reserve(64);
		SHA512(_pass, size, hash);
		char* buffer;
		int temp;
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
	}

	ind = find_user(login);
	if (ind == -1)
	{
		send_data(currentSocket, "ERROR! Invalid login!\n");

		it = ranges::find(count, currentSocket, &pair<int, SOCKET>::second);
		if (it != count.end())
			it->first++;
		else
		{
			pair<int, SOCKET> c = { 1, currentSocket };
			count.push_back(c);
		}

		log(AUTH_m, currentSocket, "Invalid login", it != count.end() ? it->first : 1);
		return -1;
	}
	if (user[ind].auth_token == true)
	{
		send_data(currentSocket, "ERROR! This user is already logged in to another thread!\n");

		s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
		s_it_g = tracked_subj.end();
		for (int i = 0; i < user[ind].group.size(); i++)
		{
			s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
			if (s_it_g != tracked_subj.end())
				break;
		}
		if (s_it->second[3] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[3] == '1'))
			log(AUTH_m, currentSocket, "Reauthorization attempt", 0);
		return -1;
	}
	else {
		if (mark > user[ind].def_mark || mark < 0)
		{
			send_data(currentSocket, "ERROR! Invalid mark!\n");
			s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
			s_it_g = tracked_subj.end();
			for (int i = 0; i < user[ind].group.size(); i++)
			{
				s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
				if (s_it_g != tracked_subj.end())
					break;
			}
			if (s_it->second[3] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[3] == '1'))
				log(AUTH_m, currentSocket, "Incorrect label of the mandatory system was passed", 0);
			return -1;
		}
		//int pos = user[ind].password.compare(pass);
		if (user[ind].password == pass)
		{
			user[ind].current_mark = mark;
			user[ind].current_path = user[ind].home_path;
			user[ind].auth_token = true;
			user[ind].cur_sock = currentSocket;
			user[ind].th_id = th_id;
			send_data(currentSocket, user[ind].home_path);
			
			it = ranges::find(count, currentSocket, &pair<int, SOCKET>::second);
			s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
			s_it_g = tracked_subj.end();
			for (int i = 0; i < user[ind].group.size(); i++)
			{
				s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
				if (s_it_g != tracked_subj.end())
					break;
			}
			if (user[ind].login != "logger" && s_it->second[3] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[3] == '1'))
				log(AUTH_m, currentSocket, "Successful authorization", it != count.end() ? it->first : 0);
			return 0;
		}
		else {
			send_data(currentSocket, "ERROR! Invalid password!\n");
			return -1;
		}
	}
}

int Server::reg(SOCKET& currentSocket, int initiator, int th_id)
{
	vector<pair<string, string>>::iterator s_it, s_it_g;
	string log, pass;
	int mark = -1;
	log.reserve(10);
	pass.reserve(20);

	//получаем из буффера команды логин и пароль
	if (cmd_buffer.size() != 0)
	{
		vector<string> box_opt = get_opt(false);
		if (box_opt.size() != 3)
		{
			send_data(currentSocket, "ERROR! Invalid command options!\n");
			return -1;
		}
		log = box_opt[0];
		pass = box_opt[1];
		mark = to_integer<int>(static_cast<byte>(box_opt[2][0]));
		mark -= '0';
	}
	else {
		send_data(currentSocket, "LOGIN->");
		get_command(currentSocket);
		log = cmd_buffer;
		send_data(currentSocket, "PASS->");
		get_command(currentSocket);
		pass = cmd_buffer;
		send_data(currentSocket, "MARK->");
		get_command(currentSocket);
		mark = std::to_integer<int>(static_cast<byte>(cmd_buffer[0]));
		mark -= '0';

		int size = pass.size();
		unsigned char* _pass;
		unsigned char hash[64];
		_pass = new unsigned char[size + 1];
		for (int i = 0; i < pass.size(); i++)
			_pass[i] = pass[i];
		pass.clear();
		pass.reserve(64);
		SHA512(_pass, size, hash);
		char* buffer;
		int temp;
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
		if (log[i] == '&' || log[i] == '$' || log[i] == '~' || log[i] == '`' || log[i] == '"' || log[i] == '%' || log[i] == '(' || log[i] == ')')
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

	int ind = find_user(currentSocket);

	//проверяем не занят ли этот логин
	if (find_user(log) != -1)
	{
		send_data(currentSocket, "ERROR! A user with this username already exists!\n");
		s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
		s_it_g = tracked_subj.end();
		for (int i = 0; i < user[ind].group.size(); i++)
		{
			s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
			if (s_it_g != tracked_subj.end())
				break;
		}
		if (s_it->second[3] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[3] == '1'))
			Server::log(AUTH_m, currentSocket, "Unsuccessful registration. Invalid parameters.", 0);
		return -1;
	}

	if (initiator == USER)
	{
		if (mark > 1)
		{
			send_data(currentSocket, "Invalid mark!\n");
			s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
			s_it_g = tracked_subj.end();
			for (int i = 0; i < user[ind].group.size(); i++)
			{
				s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
				if (s_it_g != tracked_subj.end())
					break;
			}
			if (s_it->second[3] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[3] == '1'))
				Server::log(AUTH_m, currentSocket, "Unsuccessful registration. Invalid input mark.", 0);
			return 0;
		}
	}
	else if (user[ind].current_mark < mark && ind != -1)
	{
		send_data(currentSocket, "Invalid mark!\n");
		s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
		s_it_g = tracked_subj.end();
		for (int i = 0; i < user[ind].group.size(); i++)
		{
			s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
			if (s_it_g != tracked_subj.end())
				break;
		}
		if (s_it->second[3] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[3] == '1'))
			Server::log(AUTH_m, currentSocket, "Unsuccessful registration. Invalid input mark.", 0);
		return 0;
	}

	//выделяем память под нового юзера, 
	//заполняем структуру и добавляем юзера в матрицу прав
	user = reallocated(1);
	if (initiator > USER)
	{
		user[users_count - 1].def_mark = mark;
		user[users_count - 1].current_mark = mark;
		user[users_count - 1].auth_token = false;
		user[users_count - 1].current_path = '\\' + log + "\\home";
		user[users_count - 1].cur_sock = NULL;
		user[users_count - 1].home_path = '\\' + log + "\\home";
		user[users_count - 1].login = log;
		user[users_count - 1].password = pass;
		user[users_count - 1].status = USER;
		user[users_count - 1].th_id = -1;
		user[users_count - 1].del_tocken = false;
 		user[users_count - 1].group.push_back(user[users_count - 1].login);
	}
	else {
		mark = USER_2;
		user[users_count - 1].def_mark = mark;
		user[users_count - 1].current_mark = mark;
		user[users_count - 1].auth_token = true;
		user[users_count - 1].current_path = '\\' + log + "\\home";
		user[users_count - 1].cur_sock = currentSocket;
		user[users_count - 1].th_id = th_id;
		user[users_count - 1].home_path = '\\' + log + "\\home";
		user[users_count - 1].login = log;
		user[users_count - 1].password = pass;
		user[users_count - 1].status = USER;
		user[users_count - 1].del_tocken = false;
		user[users_count - 1].group.push_back(user[users_count - 1].login);
	}

	fs::create_directories(SERVER_ROOT_PATH + user[users_count - 1].current_path);
	string access = "700" + user[users_count - 1].login + "/" + user[users_count - 1].login,
		path = SERVER_ROOT_PATH;
	path += user[users_count - 1].current_path;
	set_level_access(users_count - 1, path, access, mark);
	ms.set_mark(user[users_count - 1].login, mark);

	if (initiator > USER)
	{
		int ind = find_user(currentSocket);
		send_data(currentSocket, "Successfull!\n#" + user[ind].current_path);
	}
	else
		send_data(currentSocket, "Successfull! \nThe standard mark of the new user is assigned: 1\n#" + user[users_count - 1].current_path);
	
	s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
	s_it_g = tracked_subj.end();
	for (int i = 0; i < user[ind].group.size(); i++)
	{
		s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
		if (s_it_g != tracked_subj.end())
			break;
	}
	if (s_it->second[3] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[3] == '1'))
		Server::log(AUTH_m, currentSocket, "Successful registration of a new user", 0);

	save_users_data();
	return 0;
}

int Server::chmod(SOCKET& currentSocket)
{
	vector<pair<string, string>>::iterator s_it, s_it_g;
	string opt[2],
		path = SERVER_ROOT_PATH;
	int ind = find_user(currentSocket);

	vector<string> box_opt = get_opt(false);
	if (box_opt.size() != 2)
	{
		send_data(currentSocket, "ERROR! Invalid command options!\n");
		return -1;
	}
	opt[0] = box_opt[0];
	opt[1] = box_opt[1];

	if (check_path(opt[0], FILE_FL) == INV_SYMBOL_IN_PATH)
	{
		send_data(currentSocket, "ERROR! Invalid symbol in path\n" + user[ind].current_path);
		return 0;
	}

	string login_owner;
	if (opt[0][0] == '\\')
		path += opt[0];
	else
		path += user[ind].current_path + "\\" + opt[0];

	string access = get_level_access(ind, path, false);

	login_owner = get_owner(path);
	if (access[0] == '4' || access[0] == '7' || access[0] == '6' || access[0] == '5')
	{
		int ind_owner = find_user(login_owner);
		//дописываем в значение доступа логин создателя и его группы через слеш
		opt[1] += user[ind_owner].login;
		for (int i = 0; i < user[ind_owner].group.size(); i++)
			opt[1] += "/" + user[ind_owner].group[i];

		change_level_access(ind_owner, path, opt[1]);
		send_data(currentSocket, "Successfully!\n#" + user[ind].current_path);
		
		s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
		s_it_g = tracked_subj.end();
		for (int i = 0; i < user[ind].group.size(); i++)
		{
			s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
			if (s_it_g != tracked_subj.end())
				break;
		}
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "DISCRETE MODEL: access is allowed!", 0);
	}
	else
	{
		send_data(currentSocket, "ERROR! You don't have enough rights to execute this command!\n" + user[ind].current_path);

		s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
		s_it_g = tracked_subj.end();
		for (int i = 0; i < user[ind].group.size(); i++)
		{
			s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
			if (s_it_g != tracked_subj.end())
				break;
		}
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "DISCRETE MODEL: access denied!", 0);
	}
	return 0;
}

int Server::help(SOCKET& currentSocket)
{
	int ind = find_user(currentSocket);
	if (cmd_buffer.size() == 0)
	{
		if (ind != -1)
			send_data(currentSocket, help_all_cmd + "\n#" + user[ind].current_path);
		else
			send_data(currentSocket, help_all_cmd + "\n");
		return 0;
	}
	string opt, result;

	vector<string> box_opt = get_opt(false);
	if (box_opt.size() != 1)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return -1;
	}
	opt = box_opt[0];

	result.reserve(50);
	for (int i = 0; i < 22; i++)
	{
		if (opt == supported_commands[i])
		{
			result = help_cmds[i];
			if (ind != -1)
				send_data(currentSocket, result + "\n#" + user[ind].current_path);
			else
				send_data(currentSocket, result + "\n");
			return 0;
		}
		else if (i == 21)
		{
			if (ind != -1)
				send_data(currentSocket, "The server does not support such a command!\n#" + user[ind].current_path);
			else
				send_data(currentSocket, "The server does not support such a command!\n");
			return 0;
		}
	}
}

int Server::createFile(SOCKET& currentSocket)
{
	vector<pair<string, string>>::iterator s_it, s_it_g;
	string file_name, _access, path = SERVER_ROOT_PATH;
	int ind = find_user(currentSocket),
		mark = -1;

	s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
	s_it_g = tracked_subj.end();
	for (int i = 0; i < user[ind].group.size(); i++)
	{
		s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
		if (s_it_g != tracked_subj.end())
			break;
	}

	vector<string> box_opt = get_opt(false);
	if (box_opt.size() < 2 || box_opt.size() > 3)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return -1;
	}
	if (box_opt.size() == 3)
	{
		file_name = box_opt[0];
		_access = box_opt[1];
		if (box_opt[2].size() > 1)
		{
			send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
			return -1;
		}
		mark = to_integer<int>(static_cast<byte>(box_opt[2][0]));
		mark -= '0';
		if (mark < 0)
		{
			send_data(currentSocket, "Invalid mark!\n");
			return 0;
		}
	}
	else {
		file_name = box_opt[0];
		_access = box_opt[1];
		mark = user[ind].def_mark;
	}

	if (file_name[0] == '\\')
		path += file_name;
	else
		path += user[ind].current_path + '\\' + file_name;

	if (check_path(file_name, FILE_FL) == INV_SYMBOL_IN_PATH)
	{
		send_data(currentSocket, "ERROR! Invalid symbol in the directory name!\n" + user[ind].current_path);
		return 0;
	}
	if (fs::exists(path))
	{
		send_data(currentSocket, "ERROR! A file with this name already exists!\n" + user[ind].current_path);
		return 0;
	}

	string access = get_level_access(ind, path, false);

	if (ms.get_mark(path) > user[ind].current_mark)
	{
		send_data(currentSocket, "MANDATE MODEL ERROR!\nYou don't have access rights!\n" + user[ind].current_path);
		
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "MANDATE MODEL: access denied", 0);
		return 0;
	}
	else {
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "MANDATE MODEL: access is allowed", 0);
	}
	ofstream file;

	if (access[0] == '0' || access[0] == '1' || access[0] == '4' || access[0] == '5')
	{
		send_data(currentSocket, "DISCRETE MODEL ERROR!\nYou don't have access rights!\n" + user[ind].current_path);
		
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "DISCRETE MODEL: access denied", 0);
		return 0;
	}
	else
	{
		file.open(path);
		file.close();
	}
	if (fs::exists(path) == false)
	{
		send_data(currentSocket, "ERROR! Created directory end with failed!\n" + user[ind].current_path);
		return 0;
	}
	else {
		if ((s_it != tracked_subj.end() && s_it->second[4] == '1') || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "DISCRETE MODEL: access is allowed", 0);
	}

	//дописываем в значение доступа логин создателя и его группы через слеш
	_access += user[ind].login;
	for (int i = 0; i < user[ind].group.size(); i++)
		_access +=  '/' + user[ind].group[i];

	set_level_access(ind, path, _access, mark);
	send_data(currentSocket, "Successfull!\n" + user[ind].current_path);

	if ((s_it != tracked_subj.end() && s_it->second[1] == '1') || (s_it_g != tracked_subj.end() && s_it_g->second[1] == '1'))
		log(WR_m, currentSocket, "The file/directory " + path + " has been created", 0);

	tracked_obj.push_back(pair<string, string>(path, "000"));
	save_attr();
	return 0;
}

int Server::deleteFD(SOCKET& currentSocket)
{
	int ind = find_user(currentSocket);
	string dir_name,
		path = SERVER_ROOT_PATH;

	vector<pair<string, string>>::iterator s_it, s_it_g;
	s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
	s_it_g = tracked_subj.end();
	for (int i = 0; i < user[ind].group.size(); i++)
	{
		s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
		if (s_it_g != tracked_subj.end())
			break;
	}

	dir_name.reserve(15);
	vector<string> box_opt = get_opt(false);
	if (box_opt.size() != 1)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return -1;
	}
	dir_name = box_opt[0];

	int fl = -1;
	if (dir_name.find(".") == MAXSIZE_T)
		fl = DIR_FL;
	else
		fl = FILE_FL;

	if (fl == DIR_FL)
	{
		if (check_path(dir_name, DIR_FL) == INV_SYMBOL_IN_PATH)
		{
			send_data(currentSocket, "ERROR! Invalid symbol in the directory name!\n" + user[ind].current_path);
			return 0;
		}
	}
	else
	{
		if (check_path(dir_name, FILE_FL) == INV_SYMBOL_IN_PATH)
		{
			send_data(currentSocket, "ERROR! Invalid symbol in the directory name!\n" + user[ind].current_path);
			return 0;
		}
	}


	if (dir_name[0] == '\\')
		path += dir_name;
	else
		path += user[ind].current_path + '\\' + dir_name;

	if (!fs::exists(path))
	{
		send_data(currentSocket, "ERROR! A directory with this name does not exist!\n" + user[ind].current_path);
		return 0;
	}
	string access = get_level_access(ind, path, false);
	if (access[0] == '0' || access[0] == '1' || access[0] == '4' || access[0] == '5')
	{
		send_data(currentSocket, "DISCRETE MODEL ERROR! \nYou don't have access rights!\n" + user[ind].current_path);
		
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "DISCRETE MODEL: access denied", 0);

		return 0;
	} 
	else {
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "DISCRETE MODEL: access is allowed", 0);
	}
	int Maccess = ms.get_mark(path);
	if (user[ind].current_mark < Maccess)
	{
		send_data(currentSocket, "MANDATE MODEL ERROR! \nYou don't have access rights!\n" + user[ind].current_path);
	
		if (s_it->second[4] == '1' || s_it_g != tracked_subj.end() && s_it_g->second[4] == '1')
			log(ACC_m, currentSocket, "MANDATE MODEL: access denied", 0);
		return 0;
	}
	else {
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "MANDATE MODEL: access is allowed", 0);
	}

	string temp = SERVER_ROOT_PATH;
	temp += user[ind].current_path;

	delete_level_access(ind, path, fl);
	fs::remove_all(path);

	//проверить что удалили директорию в которой не находимся
	if (temp == path)
	{
		user[ind].current_path = user[ind].home_path;
		send_data(currentSocket, "Successfully!\n#" + user[ind].current_path);
		s_it = ranges::find(tracked_obj, path, &pair<string, string>::first);

		if (s_it->second[1] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[1] == '1'))
			log(WR_m, currentSocket, "Deleted folder/directory.", 0);
		return 0;
	}
	send_data(currentSocket, "Successfully!\n#" + user[ind].current_path);
	s_it = ranges::find(tracked_obj, path, &pair<string, string>::first);

	if ((s_it != tracked_obj.end() && s_it->second[1] == '1') || (s_it_g != tracked_subj.end() && s_it_g->second[1] == '1'))
		log(WR_m, currentSocket, "Deleted folder/directory.", 0);

	if (s_it != tracked_obj.end())
		tracked_obj.erase(s_it);
	save_attr();
	return 0;
}

int Server::createDirectory(SOCKET& currentSocket)
{
	vector<pair<string, string>>::iterator s_it, s_it_g;
	int ind = find_user(currentSocket),
		mark = -1;
	string dir_name, opt_0, opt_1,
		path = SERVER_ROOT_PATH;

	s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
	s_it_g = tracked_subj.end();
	for (int i = 0; i < user[ind].group.size(); i++)
	{
		s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
		if (s_it_g != tracked_subj.end())
			break;
	}

	dir_name.reserve(15);
	vector<string> box_opt = get_opt(false);
	if (box_opt.size() < 2 || box_opt.size() > 3)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return -1;
	}
	if (box_opt.size() == 3)
	{
		opt_0 = box_opt[0];
		opt_1 = box_opt[1];
		if (box_opt[2].size() > 1)
		{
			send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
			return -1;
		}
		mark = to_integer<int>(static_cast<byte>(box_opt[2][0]));
		mark -= '0';
		if (mark > user[ind].def_mark)
		{
			send_data(currentSocket, "Invalid mark!\n");
			return 0;
		}
	}
	else {
		opt_0 = box_opt[0];
		opt_1 = box_opt[1];
		mark = user[ind].def_mark;
	}

	if (check_path(opt_0, DIR_FL) == INV_SYMBOL_IN_PATH)
	{
		send_data(currentSocket, "ERROR! Invalid symbol in the directory name!\n" + user[ind].current_path);
		return 0;
	}
	dir_name = opt_0;


	if (dir_name[0] == '\\')
		path += dir_name;
	else
		path += user[ind].current_path + '\\' + dir_name;

	//существование директории с таким именем
	if (fs::exists(path))
	{
		send_data(currentSocket, "ERROR! A directory with this name already exists!\n" + user[ind].current_path);
		return 0;
	}
	//наличие прав на создание в этой директории
	string access = get_level_access(ind, path, false);
	int Maccess = ms.get_mark(path);
	if (Maccess > user[ind].current_mark)
	{
		send_data(currentSocket, "MANDATE MODEL ERROR!\nYou don't have access rights!\n" + user[ind].current_path);

		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "MANDATE MODEL: access denied.", 0);
		return 0;
	}
	else {
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "MANDATE MODEL: access is allowed.", 0);
	}

	if (access[0] == '0' || access[0] == '1' || access[0] == '4' || access[0] == '5')
	{
		send_data(currentSocket, "DISCRETE MODEL ERROR! \nYou don't have access rights!\n" + user[ind].current_path);

		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "DISCRETE MODEL: access denied.", 0);
		return 0;
	}
	else {
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "DISCRETE MODEL: access is allowed.", 0);
	}

	if (fs::create_directories(path) == false)
	{
		send_data(currentSocket, "ERROR! Created directory end with failed!\n" + user[ind].current_path);
		return 0;
	}
	else
	{
		switch (static_cast<char>(access[0]))
		{
		case '7': //полный доступ
			user[ind].current_path += "\\" + dir_name;
			break;
		case '6': //не разрешено просматривать
			user[ind].current_path += "\\" + dir_name;
			break;
		case '3': //нет дотупа на переход в директорию
			break;
		case '2': //нет доступа на переход и просмотр
			break;
		default:
			cout << "\n\t\t\t\aInternal error! Incorrect access rights value !\n";
			break;
		}
	}

	//дописываем в значение доступа логин создателя и его группы через слеш
	for (int i = 0; i < user[ind].group.size(); i++)
	{
		if (i == 0)
		{
			opt_1 += user[ind].group[i];
			continue;
		}
		opt_1 += "/" + user[ind].group[i];
	}

	set_level_access(ind, path, opt_1, mark);
	send_data(currentSocket, user[ind].current_path.c_str());

	tracked_obj.push_back(pair<string, string>(path, "000"));
	save_attr();
	return 0;
}

int Server::cd(SOCKET& currentSocket)
{
	vector<pair<string, string>>::iterator s_it, s_it_g;
	string opt, path = SERVER_ROOT_PATH;
	int ind = find_user(currentSocket);

	s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
	s_it_g = tracked_subj.end();
	for (int i = 0; i < user[ind].group.size(); i++)
	{
		s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
		if (s_it_g != tracked_subj.end())
			break;
	}
	
	if (cmd_buffer.size() == 0)
	{
		user[ind].current_path = user[ind].home_path;
		send_data(currentSocket, "#"+user[ind].current_path);
		return 0;
	}
	else {
		vector<string> box_opt = get_opt(false);
		if (box_opt.size() != 1)
		{
			send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
			return -1;
		}
		opt = box_opt[0];

		if (check_path(opt, DIR_FL) == INV_SYMBOL_IN_PATH)
		{
			send_data(currentSocket, "ERROR! Invalid symbol in the directory name!\n#" + user[ind].current_path);
			return 0;
		}
		if (opt[0] == '\\')
			path += opt;
		else
			path += user[ind].current_path + "\\" + opt;

		//проверить существование такой директории
		if (fs::exists(path) == false)
		{
			send_data(currentSocket, "ERROR! A directory with this name already exists!\n#" + user[ind].current_path);
			return 0;
		}
		//проверить доступ к директории в матрице прав
		string access = get_level_access(ind, path, false);

		if (access[0] == '4' || access[0] == '7' || access[0] == '6' || access[0] == '5')
		{
			int mark = ms.get_mark(path);
			if (user[ind].current_mark < mark)
			{
				send_data(currentSocket, "MANDATE MODEL ERROR! \nYou don't have access rights to this directory!\n#" + user[ind].current_path);

				if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
					log(ACC_m, currentSocket, "MANDATE MODEL: access denied.", 0);
				return 0;
			}
			else {
				if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
					log(ACC_m, currentSocket, "MANDATE MODEL: access is allowed.", 0);
			}
			if (opt[0] == '\\')
				user[ind].current_path = "#" + opt;
			else
				user[ind].current_path += "\\" + opt;
			send_data(currentSocket, user[ind].current_path);

			if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
				log(ACC_m, currentSocket, "DISCRETE MODEL: access is allowed.", 0);
		}
		else
		{
			send_data(currentSocket, "DISCRETE MODEL ERROR! \nYou don't have access rights to this directory!\n#" + user[ind].current_path);
		
			if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
				log(ACC_m, currentSocket, "DISCRETE MODEL: access denied.", 0);
		}
	}
	return 0;
}

int Server::createGroup(SOCKET& currentSocket)
{
	vector<pair<string, string>>::iterator s_it, s_it_g;

	int ind = find_user(currentSocket),
		us_ind;
	
	s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
	for (int i = 0; i < user[ind].group.size(); i++)
	{
		s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
		if (s_it_g != tracked_subj.end())
			break;
	}

	string group, login;
	vector<string> box_opt = get_opt(false);
	if (box_opt.size() != 1 && box_opt.size() != 2)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return -1;
	}
	if(box_opt.size() == 1)
		group = box_opt[0];
	else
	{
		login = box_opt[0];
		group = box_opt[1];

		us_ind = find_user(login);
		if (us_ind == -1)
		{
			send_data(currentSocket, "ERROR! There is no user with that name!\n#" + user[ind].current_path);
			return 0;
		}
		if (user[ind].current_mark < user[us_ind].def_mark)
		{
			send_data(currentSocket, "MANDATE MODEL ERROR!\nYou don't have enough rights to add this user to the group!\n#" + user[ind].current_path);
			
			if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
				log(ACC_m, currentSocket, "MANDATE MODEL: access denied.", 0);
			return 0;
		}
	}
	//проверка на уже существующую группу с таким именем
	for (int i = 0; i < users_count; i++)
	{
		for (int j = 0; j < user[i].group.size(); j++)
		{
			if (user[i].group[j] == group) {
				send_data(currentSocket, "ERROR! A group with that name already exists!\n#" + user[ind].current_path);
				return 0;
			}
		}
	}
	if(login.empty()){
		user[ind].group.push_back(group);
		update_level_access(ind);
		add_level_access(ind, group);
		ms.set_mark(group, user[ind].current_mark);
	}
	else {
		user[us_ind].group.push_back(group);
		update_level_access(us_ind);
		add_level_access(us_ind, group);
		ms.set_mark(group, user[us_ind].current_mark);
	}

	save_users_data();
	send_data(currentSocket, "Successfull!\n#" + user[ind].current_path);
	if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
		log(ACC_m, currentSocket, "MANDATE MODEL: access is allowed.", 0);
	tracked_obj.push_back(pair<string, string>(group, "000"));

	return 0;
}

int Server::deleteGroup(SOCKET& currentSocket)
{
	int ind = find_user(currentSocket);
	vector<pair<string, string>>::iterator s_it, s_it_g;
	s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
	s_it_g = tracked_subj.end();
	for (int i = 0; i < user[ind].group.size(); i++)
	{
		s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
		if (s_it_g != tracked_subj.end())
			break;
	}

	string group;
	vector<string> box_opt = get_opt(false);
	if (box_opt.size() != 1)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return 0;
	}
	group = box_opt[0];
	int mark = ms.get_mark(group);
	if (mark == -1)
	{
		send_data(currentSocket, "MANDATE MODEL ERROR! \nKey not found!\n#" + user[ind].current_path);
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "MANDATE MODEL: access denied.", 0);
		return 0;
	}
	if (user[ind].current_mark < mark)
	{
		send_data(currentSocket, "MANDATE MODEL ERROR! \nThere are not enough rights to delete such a group!\n#" + user[ind].current_path);
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "MANDATE MODEL: access denied.", 0);
		return 0;
	}

	vector<string>::iterator it;
	for (int i = 0; i < users_count; i++)
	{
		it = find(user[i].group.begin(), user[i].group.end(), group);
		if (it != user[i].group.end())
		{
			user[i].group.erase(it);
			update_level_access(i);
		}
	}
	ms.delete_marks(group);
	
	if (group != user[ind].login)
	{
		map<pair<string, string>, string>::iterator Lit = LMatrix.begin();
		for (Lit; Lit != end(LMatrix); Lit++)
		{
			if (Lit->first.first == group)
				Lit = LMatrix.erase(Lit);
		}
	}
	send_data(currentSocket, "Successfull!\n#" + user[ind].current_path);
	if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
		log(ACC_m, currentSocket, "MANDATE MODEL: access is allowed.", 0);

	tracked_subj.erase(s_it);
	return 0;
}

int Server::addUserInGroup(SOCKET& currentSocket)
{
	int ind = find_user(currentSocket);
	vector<pair<string, string>>::iterator s_it, s_it_g;
	s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
	s_it_g = tracked_subj.end();
	for (int i = 0; i < user[ind].group.size(); i++)
	{
		s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
		if (s_it_g != tracked_subj.end())
			break;
	}

	string group, _user;
	vector<string> box_opt = get_opt(false);
	if (box_opt.size() != 2)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return 0;
	}
	_user = box_opt[0];
	group = box_opt[1];

	int user_ind = find_user(_user);
	if (user_ind == -1)
	{
		send_data(currentSocket, "ERROR! A user with this name was not found!\n#" + user[ind].current_path);
		return 0;
	}
	for (int i = 0; i < users_count; i++)
	{
		string temp;
		for (int j = 0; j < user[i].group.size(); j++)
		{
			if (user[i].group[j] == group)
				temp = group;
		}
		if (!temp.empty())
			break;
		else if (i == users_count - 1)
		{
			int _m = ms.get_mark(group);
			if (_m == -1)
			{
				send_data(currentSocket, "ERROR! There is no such group!\n#" + user[ind].current_path);
				return 0;
			}
		}
	}

	int mark = ms.get_mark(group);
	if (mark == -1)
	{
		send_data(currentSocket, "MANDATE MODEL ERROR! \nKey not found!\n#" + user[ind].current_path);
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "MANDATE MODEL: access denied.", 0);
		return 0;
	}
	if (user[ind].current_mark < mark)
	{
		send_data(currentSocket, "MANDATE MODEL ERROR! \nNot enough rights to add to this group!\n#" + user[ind].current_path);
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "MANDATE MODEL: access denied.", 0);
		return 0;
	}
	
	user[user_ind].group.push_back(group);
	update_level_access(user_ind);
	add_level_access(user_ind, group);
	save_users_data();
	send_data(currentSocket, "Successfull!\n#" + user[ind].current_path);
	if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
		log(ACC_m, currentSocket, "MANDATE MODEL: access is allowed.", 0);

	tracked_subj.push_back(pair<string, string>(_user, "00000"));

	return 0;
}

int Server::deleteUserInGroup(SOCKET& currentSocket)
{
	int ind = find_user(currentSocket);
	vector<pair<string, string>>::iterator s_it, s_it_g;
	s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
	s_it_g = tracked_subj.end();
	for (int i = 0; i < user[ind].group.size(); i++)
	{
		s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
		if (s_it_g != tracked_subj.end())
			break;
	}

	string group, _user;
	vector<string> box_opt = get_opt(false);
	if (box_opt.size() != 2)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return 0;
	}
	_user = box_opt[0];
	group = box_opt[1];

	int user_ind = find_user(_user);
	if (user_ind == -1)
	{
		send_data(currentSocket, "ERROR! A user with this name was not found!\n#" + user[ind].current_path);
		return 0;
	}
	
	//итерируемся по матрице и если не находим нужную группу, то выдаем сообщение об этом
	//Нужно удалять группу если в ней нет ни одного юзера (ф-я )
	map<pair<string, string>, string>::iterator _it;
	for (_it = LMatrix.begin(); _it != LMatrix.end(); _it++)
	{
		if (_it->first.first == group)
			break;
	}
	if (_it == LMatrix.end())
	{
		send_data(currentSocket, "ERROR! There is no such group!\n#" + user[ind].current_path);
		return 0;
	}
	
	if (user[ind].current_mark < user[user_ind].def_mark)
	{
		send_data(currentSocket, "MANDATE MODEL ERROR! \nYou do not have the rights to execute this command for this user!\n#" + user[ind].current_path);
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "MANDATE MODEL: access denied.", 0);
		return 0;
	}
	else {
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "MANDATE MODEL: access is allowed.", 0);
	}

	

	vector<string>::iterator it;
	it = find(user[user_ind].group.begin(), user[user_ind].group.end(), group);
	if (it == user[user_ind].group.end())
	{
		send_data(currentSocket, "ERROR! A group with that name was not found!\nThe command is not executed.\n#" + user[ind].current_path);
		return 0;
	}

	int i = 0, count =0;
	for (i; i<users_count; i++)
	{
		if (find(user[i].group.begin(), user[i].group.end(), group) != user[i].group.end())
		{
			count++;
		}
	}
	if (count != 0)
	{
		for (_it = LMatrix.begin(); _it != LMatrix.end(); _it++)
		{
			if (_it->first.first == group)
			{
				string owner = get_owner(_it->first.second);
				if (owner == user[user_ind].login)
				{
					_it = LMatrix.erase(_it);
					_it--;
				}
			}
		}
		//if(count == 1)
		//	ms.delete_marks(group);
	}
	user[user_ind].group.erase(it);
	//update matrix 
	update_level_access(user_ind);

	//save_Matrix_Law();
	send_data(currentSocket, "Successfull!\n#" + user[ind].current_path);

	return 0;
}

int Server::write(SOCKET& currentSocket, bool APP_FL)
{
	string data, file_name;
	int ind = find_user(currentSocket),
		mark = -1;
	vector<pair<string, string>>::iterator s_it, s_it_g;
	s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
	s_it_g = tracked_subj.end();
	for (int i = 0; i < user[ind].group.size(); i++)
	{
		s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
		if (s_it_g != tracked_subj.end())
			break;
	}

	vector<string> box_opt = get_opt(false);
	if (box_opt.size() != 2)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return -1;
	}
	file_name = box_opt[0];
	data = box_opt[1];

	if (check_path(file_name, FILE_FL) == INV_SYMBOL_IN_PATH)
	{
		send_data(currentSocket, "ERROR! Invalid symbol in the file name!\n" + user[ind].current_path);
		return 0;
	}

	string path = SERVER_ROOT_PATH;
	if (file_name[0] == '\\')
		path += file_name;
	else
		path += user[ind].current_path + "\\" + file_name;
	//проверить существование такого файла
	if (fs::exists(path) == false)
	{
		send_data(currentSocket, "ERROR! A file with this name already exists!\n#" + user[ind].current_path);
		return 0;
	}
	//проверить доступ к файлу в матрице прав
	string access = get_level_access(ind, path, false);
	if (access[0] == '2' || access[0] == '3' || access[0] == '6' || access[0] == '7')
	{
		fstream file;
		if (!APP_FL)
		{
			mark = ms.get_mark(path);
			if (user[ind].current_mark != mark)
			{
				send_data(currentSocket, "MANDATE MODEL ERROR! You don't have append access to this file!\n#" + user[ind].current_path);
				if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
					log(ACC_m, currentSocket, "MANDATE MODEL: access denied.", 0);
				return 0;
			}
			mark = to_integer<int>(static_cast<byte>(access[1])) - '0';
			if (user[ind].current_mark != mark && mark != -1)
			{
				send_data(currentSocket, "MANDATE MODEL ERROR! You don't have append access to this file!\n#" + user[ind].current_path);
				if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
					log(ACC_m, currentSocket, "MANDATE MODEL: access denied.", 0);
				return 0;
			}
			if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
				log(ACC_m, currentSocket, "MANDATE MODEL: access is allowed.", 0);
			file.open(path, ios::out);
		}
		else {
			mark = ms.get_mark(path);
			if (user[ind].current_mark > mark && mark != -1)
			{
				send_data(currentSocket, "MANDATE MODEL ERROR! You don't have write access to this file!\n#" + user[ind].current_path);
				if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
					log(ACC_m, currentSocket, "MANDATE MODEL: access denied.", 0);
				return 0;
			}
			mark = to_integer<int>(static_cast<byte>(access[1])) - '0';
			if (user[ind].current_mark > mark && mark != -1)
			{
				send_data(currentSocket, "MANDATE MODEL ERROR! You don't have write access to this file!\n#" + user[ind].current_path);
				if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
					log(ACC_m, currentSocket, "MANDATE MODEL: access denied.", 0);
				return 0;
			}
			if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
				log(ACC_m, currentSocket, "MANDATE MODEL: access is allowed.", 0);
			file.open(path, ios::app);
		}

		if (!file.is_open())
		{
			send_data(currentSocket, "Internal server error! Couldn't open the file " + file_name + "\n#" + user[ind].current_path);
			return 0;
		}
		file << data;
		file.close();
		send_data(currentSocket, "successfully!\n#" + user[ind].current_path);

		/*if (s_it->second[2] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[2] == '1'))
			log(APP_m, currentSocket, path, 0);*/

		if (APP_FL)
		{
			if ((s_it != tracked_subj.end() && s_it->second[4] == '1') || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
				log(ACC_m, currentSocket, "DISCRETE MODEL: access is allowed.", 0);

			if (s_it->second[2] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[2] == '1'))
				log(APP_m, currentSocket, path, 0);

			vector<pair<string, string>>::iterator it = ranges::find(tracked_obj, path,
				&std::pair<std::string, string>::first);
			if (it != tracked_obj.end())
			{
				if (it->second[2] == '1')
					log(APP_m, currentSocket, path, 0);
			}
		}
		else {
			if ((s_it != tracked_subj.end() && s_it->second[4] == '1') || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
				log(ACC_m, currentSocket, "DISCRETE MODEL: access is allowed.", 0);

			if (s_it->second[1] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[1] == '1'))
				log(WR_m, currentSocket, path, 0);

			vector<pair<string, string>>::iterator it = ranges::find(tracked_obj, path,
				&std::pair<std::string, string>::first);
			if (it != tracked_obj.end())
			{
				if (it->second[1] == '1')
					log(WR_m, currentSocket, path, 0);
			}
		}
		
	}
	else
	{
		send_data(currentSocket, "DISCRETE MODEL ERROR! You don't have write access to this file!\n#" + user[ind].current_path);
		if ((s_it != tracked_subj.end() && s_it->second[4] == '1') || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "DISCRETE MODEL: access denied.", 0);
		return 0;
	}

	return 0;
}

int Server::read(SOCKET& currentSocket)
{
	int ind = find_user(currentSocket),
		mark = -1;
	vector<pair<string, string>>::iterator s_it, s_it_g;
	s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
	s_it_g = tracked_subj.end();
	for (int i = 0; i < user[ind].group.size(); i++)
	{
		s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
		if (s_it_g != tracked_subj.end())
			break;
	}

	string data, file_name, tmp;
	vector<string> box_opt = get_opt(false);
	if (box_opt.size() != 1)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return -1;
	}
	file_name = box_opt[0];

	if (check_path(file_name, FILE_FL) == INV_SYMBOL_IN_PATH)
	{
		send_data(currentSocket, "ERROR! Invalid symbol in the file name!\n" + user[ind].current_path);
		return 0;
	}

	string path = SERVER_ROOT_PATH;
	if (file_name[0] == '\\')
		path += file_name;
	else
		path += user[ind].current_path + "\\" + file_name;
	//проверить существование такого файла
	if (fs::exists(path) == false)
	{
		send_data(currentSocket, "ERROR! A file with this name already exists!\n#" + user[ind].current_path);
		return 0;
	}

	//проверить доступ к файлу в матрице прав
	string access = get_level_access(ind, path, false);
	if (access[0] == '1' || access[0] == '3' || access[0] == '5' || access[0] == '7')
	{
		mark = ms.get_mark(path);
		if (user[ind].current_mark < mark)
		{
			send_data(currentSocket, "MANDATE MODEL ERROR! You don't have access to this file!\n#" + user[ind].current_path);
			if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
				log(ACC_m, currentSocket, "MANDATE MODEL: access denied", 0);
			return 0;
		}
		int cur_mark = to_integer<int>(static_cast<byte>(access[1])) - '0';
		if (user[ind].current_mark < cur_mark)
		{
			send_data(currentSocket, "MANDATE MODEL ERROR! You don't have access to this file!\n#" + user[ind].current_path);
			if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
				log(ACC_m, currentSocket, "MANDATE MODEL: access denied.", 0);
			return 0;
		}
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "MANDATE MODEL: access is allowed.", 0);
		fstream file;
		file.open(path, ios::in);
		if (!file.is_open())
		{
			send_data(currentSocket, "Internal server error! Couldn't open the file " + file_name + "\n#" + user[ind].current_path);
			return 0;
		}
		data.reserve(1000);
		tmp.reserve(100);
		while (!file.eof())
		{
			file >> tmp;
			data += tmp;
		}
		file.close();
		send_data(currentSocket, data + "\n#" + user[ind].current_path);

		if ((s_it != tracked_subj.end() && s_it->second[4] == '1') || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "DISCRETE MODEL: access is allowed.", 0);

		vector<pair<string, string>>::iterator it = ranges::find(tracked_obj, path,
			&std::pair<std::string, string>::first);
		if (it != tracked_obj.end())
		{
			if(it->second[0] == '1')
				log(R_m, currentSocket, path, 0);
		}
		if (s_it->second[0] == '1' || s_it_g->second[0] == '1')
			log(R_m, currentSocket, path, 0);


		return 0;
	}
	else
	{
		send_data(currentSocket, "DISCRETE MODEL ERROR! You don't have read access to this file!\n#" + user[ind].current_path);
		if ((s_it != tracked_subj.end() && s_it->second[4] == '1') || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "DISCRETE MODEL: access denied.", 0);
		return 0;
	}

	return 0;
}

int Server::ls(SOCKET& currentSocket)
{
	map<pair<string, string>, string>::iterator it;
	vector<string> box;
	string path, opt;
	int ind = find_user(currentSocket);
	vector<pair<string, string>>::iterator s_it, s_it_g;
	s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
	s_it_g = tracked_subj.end();
	for (int i = 0; i < user[ind].group.size(); i++)
	{
		s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
		if (s_it_g != tracked_subj.end())
			break;
	}

	path.reserve(50);
	path = SERVER_ROOT_PATH;
	if (cmd_buffer.size() != 0)
	{
		vector<string> box_opt = get_opt(false);
		if (box_opt.size() != 1)
		{
			send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
			return -1;
		}
		opt = box_opt[0];

		if (opt[0] == '\\')
			path += opt;
		else
			path += user[ind].current_path + '\\' + opt;
	}
	else
		path += user[ind].current_path;

	string access = get_level_access(ind, path, false);
	if (access[0] == '2' || access[0] == '4' || access[0] == '6')
	{
		send_data(currentSocket, "DISCRETE MODEL ERROR!\nNot enough rights to view this directory!\n#" + user[ind].current_path);
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "DISCRETE MODEL: access denied.", 0);
		return 0;
	}
	else {
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "DISCRETE MODEL: access is allowed.", 0);
	}

	int mark = ms.get_mark(path);
	if (mark == -1)
	{
		send_data(currentSocket, "MANDATE MODEL ERROR! \nKey was not found!\n#" + user[ind].current_path);
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "MANDATE MODEL: access denied.", 0);
		return 0;
	}
	if (user[ind].current_mark < mark)
	{
		send_data(currentSocket, "MANDATE MODEL ERROR!\nNot enough rights to view this directory!\n#" + user[ind].current_path);
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "MANDATE MODEL: access denied.", 0);
		return 0;
	}
	else {
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "MANDATE MODEL: access is allowed.", 0);
	}

	if (check_path(opt, DIR_FL) == INV_SYMBOL_IN_PATH)
	{
		send_data(currentSocket, "ERROR! Invalid symbol in the directory name!\n" + user[ind].current_path);
		return 0;
	}

	if (!fs::exists(path))
	{
		send_data(currentSocket, "ERROR! A directory with this name already exists!\n#" + user[ind].current_path);
		return 0;
	}

	string erase_temp, out_str;
	out_str.reserve(100);

	for (auto const& dir_entry : std::filesystem::directory_iterator{ path })
	{
		erase_temp = dir_entry.path().string();
		for (int i = 0; i < erase_temp.size(); i++)
		{
			if (i >= 40)
				out_str.insert(out_str.end(), erase_temp[i]);
		}
		out_str.insert(out_str.end(), '\n');
	}

	send_data(currentSocket, out_str + "#" + user[ind].current_path);

	return 0;
}

int Server::logout(SOCKET& currentSocket, int th_id)
{
	int cl_id = find_user(currentSocket);
	send_data(currentSocket, "#0");
	user[cl_id].auth_token = false;

	//закрываем клиентский сокет
	close_client(currentSocket, th_id, cl_id);

	//сохраняем данные юзеров
	save_users_data();

	user[cl_id].cur_sock = 0;
	vector<pair<string, string>>::iterator s_it, s_it_g;
	s_it = ranges::find(tracked_subj, user[cl_id].login, &pair<string, string>::first);
	s_it_g = tracked_subj.end();
	for (int i = 0; i < user[cl_id].group.size(); i++)
	{
		s_it_g = ranges::find(tracked_subj, user[cl_id].group[i], &pair<string, string>::first);
		if (s_it_g != tracked_subj.end())
			break;
	}
	if ((s_it!= tracked_subj.end() && s_it->second[3] == '1') || (s_it_g != tracked_subj.end() && s_it_g->second[3] == '1'))
		log(AUTH_m, currentSocket, "The user " + user[cl_id].login + " is disabled", 0);
	//добавляем освободившийся индекс в стек
	free_index.push(th_id);
	return EXIT;
}

int Server::createUser(SOCKET& currentSocket, int th_id)
{
	int ind = find_user(currentSocket);
	if (user[ind].status < ADMIN)
	{
		send_data(currentSocket, "ERROR! To execute this command, you must have an ADMIN access level and higher.\n#" + user[ind].current_path);
		return 0;
	}
	reg(currentSocket, user[ind].status, th_id);

	return 0;
}

int Server::deleteUser(SOCKET& currentSocket, int th_id)
{
	int ind = find_user(currentSocket);

	if (user[ind].status < ADMIN)
	{
		send_data(currentSocket, "ERROR! To execute this command, you must have an ADMIN access level and higher.\n#" + user[ind].current_path);
		return 0;
	}

	string login;
	vector<string> box_opt = get_opt(false);
	if (box_opt.size() != 1)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return -1;
	}
	login = box_opt[0];

	int del_ind = find_user(login);
	if (del_ind == -1)
	{
		send_data(currentSocket, "ERROR! There is no user with this username!\n#" + user[ind].current_path);
		return 0;
	}

	string del_path = SERVER_ROOT_PATH;
	del_path += '\\' + user[del_ind].login;
	if (!fs::exists(del_path))
	{
		send_data(currentSocket, "Internal server error! \nThe root directory of the user being deleted was not found!\n#" + user[ind].current_path);
		return 0;
	}
	//удаляем все права для данного юзера (права по ключу юзера)
	delete_level_access(del_ind, del_path, DIR_FL);
	

	fs::remove_all(del_path);
	if (!user[ind].auth_token) 
		cout << "\a\tWARNING!!! The connection with the user who requested the execution of the command 'delete user' is lost!" << endl;
	int ret_value = 0;
	if (user[del_ind].auth_token == true)
	{
		SOCKET temp = user[del_ind].cur_sock;
		user[del_ind].del_tocken = true;
		send_data(temp, "#1");
		user[del_ind].auth_token = false;
		shutdown(temp, 2);
		closesocket(temp);
		user[del_ind].cur_sock = 0;
		free_index.push(user[del_ind].th_id);
	}
	user = reallocated(-1);
	save_users_data();
	send_data(currentSocket, "Successfull!\n" + user[ind].current_path);

	return 0;
}

int Server::changePUser(SOCKET& currentSocket)
{
	int ind = find_user(currentSocket);
	if (user[ind].status < ADMIN)
	{
		send_data(currentSocket, "ERROR! To execute this command, you must have an ADMIN access level and higher.\n#" + user[ind].current_path);
		return 0;
	}

	string login,pass;

	if (cmd_buffer.size() != 0)
	{
		vector<string> box_opt = get_opt(true);
		if (box_opt.size() != 2)
		{
			send_data(currentSocket, "ERROR! Invalid command options!\n");
			return -1;
		}
		login = box_opt[0];
		pass = box_opt[1];
	}
	else {
		send_data(currentSocket, "LOGIN->");
		get_command(currentSocket);
		login = cmd_buffer;
		send_data(currentSocket, "PASS->");
		get_command(currentSocket);
		pass = cmd_buffer;

		int size = pass.size();
		unsigned char* _pass;
		unsigned char hash[64];
		_pass = new unsigned char[size + 1];
		for (int i = 0; i < pass.size(); i++)
			_pass[i] = pass[i];
		pass.clear();
		pass.reserve(64);
		SHA512(_pass, size, hash);
		char* buffer;
		int temp;
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

	}
	
	int user_ind = find_user(login);
	if (user_ind == -1)
	{
		send_data(currentSocket, "ERROR! There is no user with this username!\n#" + user[ind].current_path);
		return 0;
	}

	//change password
	user[user_ind].password = pass;
	save_users_data();
	send_data(currentSocket, "Successfull!\n#" + user[ind].current_path);
	return 0;

}

int Server::changeLUser(SOCKET& currentSocket)
{
	int ind = find_user(currentSocket);

	if (cmd_buffer.size() == 0)
	{
		send_data(currentSocket, "Invalid command options!\n#" + user[ind].current_path);
		return 0;
	}
	vector<string> opt = get_opt(false);
	int ch_ind = find_user(opt[0]);
	if (user[ch_ind].status > user[ind].status || user[ind].status == USER)
	{
		send_data(currentSocket, "You don't have the rights to execute this command!\n#" + user[ind].current_path);
		return 0;
	}
	if (user[ch_ind].login == user[ind].login)
	{
		send_data(currentSocket, "You can't change your own status!\n#" + user[ind].current_path);
		return 0;
	}
	int new_status = stoi(opt[1]);
	if (user[ind].status == ADMIN)
	{
		if (new_status > 1)
		{
			send_data(currentSocket, "You don't have enough rights to set this status!\n#" + user[ind].current_path);
			return 0;
		}
	}
	if (new_status > 2)
	{
		send_data(currentSocket, "Invalid value!\n#" + user[ind].current_path);
		return 0;
	}
	user[ch_ind].status = new_status;
	send_data(currentSocket, "Successfull!\n#" + user[ind].current_path);
	return 0;
}

int Server::listUser(SOCKET& currentSocket)
{
	int ind = find_user(currentSocket);
	if (user[ind].status < ADMIN)
	{
		send_data(currentSocket, "ERROR! To execute this command, you must have an ADMIN access level and higher.\n#" + user[ind].current_path);
		return 0;
	}
	string result;

	result.reserve(300);
	for (int i = 1; i < users_count; i++)
	{
		result += "\n----------------------\n";
		result += "\nLOGIN: " + user[i].login;
		if (user[i].status == USER)
			result += "\nSTATUS: user";
		else if (user[i].status == ADMIN)
			result += "\nSTATUS: admin";
		if (user[i].auth_token)
			result += "\nAUTHORIZE: authorize";
		else
			result += "\nAUTHORIZE: not authorize ";
		result += "\nDEF. MARK: " + to_string(user[i].def_mark);

		result += "\nCURR. SOCKET: " + to_string(user[i].cur_sock);
		result += "\nUSERS GROUPS : ";
		for (int j = 0; j < user[i].group.size(); j++)
		{
			result += user[i].group[j] + " ";
		}
	}
	result += "\n###################################";
	send_data(currentSocket, result + "\n#" + user[ind].current_path);
	return 0;
}

int Server::rr(SOCKET& currentSocket)
{
	int ind = find_user(currentSocket);
	string fname,
		path = SERVER_ROOT_PATH;

	vector<string> box_opt = get_opt(false);
	if (box_opt.size() != 1)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return -1;
	}
	fname = box_opt[0];

	if (check_path(fname, FILE_FL) == INV_SYMBOL_IN_PATH)
	{
		send_data(currentSocket, "ERROR! Invalid symbol in the directory name!\n" + user[ind].current_path);
		return 0;
	}

	if (fname[0] == '\\')
		path += fname;
	else
		path += user[ind].current_path + '\\' + fname;

	//существование директории с таким именем
	if (!fs::exists(path))
	{
		send_data(currentSocket, "ERROR! There is no directory/file on this path!\n" + user[ind].current_path);
		return 0;
	}

	string access = get_level_access(ind, path, true);
	access.erase(3);

	send_data(currentSocket, "Access rights for this path: " + access + "\n#" + user[ind].current_path);
	return 0;
}

int Server::cm(SOCKET& currentSocket)
{
	int ind = find_user(currentSocket),
		mark = -1;
	vector<string> box_opt = get_opt(false);
	if (box_opt.size() != 1)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return -1;
	}
	string object = box_opt[0];

	int _ind = find_user(object);
	if (_ind != -1)
	{
		if (user[_ind].auth_token)
			mark = user[_ind].current_mark;
		else
			mark = user[_ind].def_mark;
		string str_mark = "Current mark for an object: ";
		str_mark += to_string(mark);
		send_data(currentSocket, str_mark + "\n#" + user[ind].current_path);
		return 0;
	}
	mark = ms.get_mark(object);
	if (mark != -1)
	{
		string str_mark = "Current mark for an object: ";
		str_mark += to_string(mark);
		send_data(currentSocket, str_mark + "\n#" + user[ind].current_path);
		return 0;
	}
	string _object = SERVER_ROOT_PATH;
	if (object[0] == '\\')
		_object += object;
	else
		_object += user[ind].current_path + '\\' + object;

	mark = ms.get_mark(_object);
	if (mark == -1)
	{
		string answer = "No mark found for the key!\n#";
		send_data(currentSocket, answer + user[ind].current_path);
		return -1;
	}
	else
	{
		string str_mark = "Current mark for an object: ";
		str_mark += to_string(mark);
		send_data(currentSocket, str_mark + "\n#" + user[ind].current_path);
		return 0;
	}
}

int Server::chM(SOCKET& currentSocket)
{
	int ind = find_user(currentSocket),
		mark = -1;
	vector<pair<string, string>>::iterator s_it, s_it_g;
	s_it = ranges::find(tracked_subj, user[ind].login, &pair<string, string>::first);
	s_it_g = tracked_subj.end();
	for (int i = 0; i < user[ind].group.size(); i++)
	{
		s_it_g = ranges::find(tracked_subj, user[ind].group[i], &pair<string, string>::first);
		if (s_it_g != tracked_subj.end())
			break;
	}

	vector<string> box_opt = get_opt(false);
	if (box_opt.size() != 2)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return -1;
	}
	if (box_opt[1].size() > 1)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return -1;
	}

	string object = box_opt[0];

	if (mark = ms.get_mark(object) != -1)
	{
		if (mark > user[ind].current_mark)
		{
			send_data(currentSocket, "MANDATE MODEL ERROR!\n It is not possible to set a mark higher than your current mark!\n#" + user[ind].current_mark);
			if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
				log(ACC_m, currentSocket, "MANDATE MODEL: access denied.", 0);
			return -1;
		}
		else {
			if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
				log(ACC_m, currentSocket, "MANDATE MODEL: access is allowed.", 0);
		}

		mark = to_integer<int>(static_cast<byte>(box_opt[1][0]));
		mark -= '0';
		ms.change_mark(object, mark);
		send_data(currentSocket, "Successfull!\n#" + user[ind].current_path);
		return 0;
	}
	int _ind = find_user(object);
	if (_ind == -1)
	{
		string msObject = SERVER_ROOT_PATH;
		if (object[0] == '\\')
			msObject += object;
		else
			msObject += user[ind].current_path + '\\' + object;
		mark = ms.get_mark(msObject);
		if (mark == -1)
		{
			send_data(currentSocket, "ERROR! There is no such object!\n#" + user[ind].current_path);
			return -1;
		}

		if (mark > user[ind].current_mark)
		{
			send_data(currentSocket, "MANDATE MODEL ERROR!\n It is not possible to set a mark higher than your current mark!\n#" + user[ind].current_mark);
			if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
				log(ACC_m, currentSocket, "MANDATE MODEL: access denied.", 0);
			return -1;
		}
		else {
			if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
				log(ACC_m, currentSocket, "MANDATE MODEL: access is allowed.", 0);
		}

		mark = to_integer<int>(static_cast<byte>(box_opt[1][0]));
		mark -= '0';
		ms.change_mark(msObject, mark);
		send_data(currentSocket, "Successfull!\n#" + user[ind].current_path);
		return 0;
	}

	if (user[ind].current_mark < user[_ind].current_mark)
	{
		send_data(currentSocket, "MANDATE MODEL ERROR! \nYou do not have access to change the label for this object/subject!\n#" + user[ind].current_path);
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "MANDATE MODEL: access denied.", 0);
		return -1;
	}
	mark = to_integer<int>(static_cast<byte>(box_opt[1][0]));
	mark -= '0';
	if (mark > user[ind].current_mark)
	{
		send_data(currentSocket, "MANDATE MODEL ERROR!\n It is not possible to set a mark higher than your current mark!\n#" + user[ind].current_mark);
		if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
			log(ACC_m, currentSocket, "MANDATE MODEL: access denied.", 0);
		return -1;
	}

	user[_ind].def_mark = mark;
	send_data(currentSocket, "Successfull!\n#" + user[ind].current_path);
	if (s_it->second[4] == '1' || (s_it_g != tracked_subj.end() && s_it_g->second[4] == '1'))
		log(ACC_m, currentSocket, "MANDATE MODEL: access is allowed.", 0);
	return 0;
}