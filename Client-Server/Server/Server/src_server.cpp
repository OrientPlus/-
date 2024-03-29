#include "src_server.h"

//Reallocates memory for the user class

Server::USERS* Server::reallocated(size_t size)
{
	USERS* new_user = new USERS[size];

	for (int i = 0; i < size-1; i++)
	{
		new_user[i].login = user[i].login;
		new_user[i].password = user[i].password;
		new_user[i].current_path = user[i].current_path;
		new_user[i].home_path = user[i].home_path;
		new_user[i].auth_token = user[i].auth_token;
		new_user[i].status = user[i].status;
		if (new_user[i].auth_token)
			new_user[i].cur_sock = user[i].cur_sock;
		for(int j=0; j<user[i].group.size(); j++)
			new_user[i].group.push_back(user[i].group[j]);
	}

	delete[] Server::user;

	return &new_user[0];
}

void Server::start()
{
	init();
	header();
	return;
}

int Server::load_users_data()
{
	//���������� ����� ??? OpenSSL ???
	ifstream file;

	file.open(USERS_DATA_PATH);
	file.seekg(1);
	if (file.bad())
	{
		cout << "\nUnable to open the file::usersData.txt\n";
		return -1;
	}

	string temp_gr;
	int i = 1;
	while (file.eof() == false)
	{
		if (i == users_count)
		{
			users_count++;
			user = reallocated(users_count);
		}
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
		/*if (user[i].login != "none")
			users_count++;*/
		file >> user[i].password;
		file >> user[i].status;

		i++;
	}
	file.close();

	return 0;
}

int Server::save_users_data()
{
	fstream file;

	file.open(USERS_DATA_PATH);
	if (file.bad())
	{
		cout << "\n\t\t\tUnable to open the file::usersData.txt  A new file will be created!\n";
		file.close();

		file.open(USERS_DATA_PATH);
	}
	file << users_count << endl;
	for (int i = 1; i < users_count; i++)
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
		file << user[i].status << endl;
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

	fstream file(USERS_DATA_PATH);
	file >> users_count;
	file.close();


	try
	{
		//user = (USERS*)malloc(users_count*sizeof(USERS));
		user = new USERS[users_count];
	}
	catch (exception ex)
	{
		cout << "\nCant allocate memory! :: " << ex.what() << endl;
	}

	//�������������� �����������������
	user[0].login = "god";
	user[0].password = "just_god";
	user[0].home_path = "/god/home";
	user[0].current_path = "/god/home";
	user[0].group.push_back("god");
	user[0].group.push_back(PRIORITY);
	user[0].auth_token = false;
	user[0].status = GOD;


	//��������� ��������� ���������� � �������������
	load_users_data();

	//��������� ������� ���� �� ����� (���������� �������������� ������� map)
	if (!loadMatrixLaw())
	{
		cout << "\n\t\t\tError loading the access matrix file!\n";
		system("pause");
		//���������� �������� �������?
		exit(-1);
	}

	//���������� ��������� ������� ��� ������� �������
	for (int i = MAX_LOAD - 1; i >= 0; i--)
		free_index.push(i);
	help_cmds[0] = "\n\t* help - displays help about commands. Parameters: help [command] \nIf the option is not set, it displays information about all commands";
	help_cmds[1] = "\n\t* reg - Registers a new client on the server with minimal rights. \nParameters: reg [login] [password]";
	help_cmds[2] = "\n\t* auth - authorization command. Parameters: auth [login] [password]";
	help_cmds[3] = "\n\t* rr - Returns the user's rights to a specific file. \nParameters: rr [filename]";
	help_cmds[4] = "\n\t* write - the command to write to a file. Parameters: write [file_name] [text]";
	help_cmds[5] = "\n\t* read - a command to read from a file. Parameters: write [file_name]";
	help_cmds[6] = "\n\t* ls - directory listing command. Parameters: ls [dir]; \nif no parameters are specified, the current directory is listed.";
	help_cmds[7] = "\n\t* logout - the command to disconnect from the server. Has no parameters";
	help_cmds[8] = "\n\t* chmod - command to set file/directory permissions. \nParameters: chmod [file name] [access value]\nExample: chmod user/home/file1.txt 777";
	help_cmds[9] = "\n\t* crF - creates a file with the specified name. Parameters: crF [file_name]";
	help_cmds[10] = "\n\t* crD - creates a directory with the specified name. Parameters: crD [path/dir_name] [rights]\nIf the path is not specified, creates in the current directory.";
	help_cmds[11] = "\n\t* del - deletes the file or directory with the specified name, if the user has the rights to do so. \nParameters: rf [file_name/directory]";
	help_cmds[12] = "\n\t* cd - go to the specified directory. Parameters: cd [path]";
	help_cmds[13] = "\n\t* crGr - create a group with the specified access rights. \nThe user who created this group is included in it by default. \nParameters: crGr [gr_name] [law for other user] [users who included in group]";
	help_cmds[14] = "\n\t* delGr - deletes the specified group if the user has the rights to do so. \nParameters: delGr [gr_name]";
	help_cmds[15] = "\n\t* addUnG - add user in group";
	help_cmds[16] = "\n\t* delUnG - del user in group";
	help_cmds[17] = "\n\t* crUser - the user creation command. Parameters: crUser [user_name] [user_password]";
	help_cmds[18] = "\n\t* delUser - the user deletion command. If a user with such a login is found, \nit deletes it, as well as its files and directories. \nParameters: delUser [user_login]";
	help_cmds[19] = "\n\t* chLUser - the command for setting user rights. Determines the user's access level on the server. \nParameters: chLUser [option]. \nPossible access levels: ADMIN, USER, WORM";
	help_cmds[20] = "\n\t* chPUser - ???";
	help_cmds[21] = "\n\t* listUser - get information about all users. Has no parameters.";

	help_all_cmd = "Supported commands:\
\n\t* help - displays help about commands. Parameters: help [command] \nIf the option is not set, it displays information about all commands\
\n\t* reg - Registers a new client on the server with minimal rights. \nParameters: reg [login] [password]\
\n\t* auth - authorization command. Parameters: auth [login] [password]\
\n\t* rr - Returns the user's rights to a specific file. \nParameters: rr [filename]\
\n\t* write - the command to write to a file. Parameters: write [file_name] [text]\
\n\t* read - a command to read from a file. Parameters: write [file_name]\
\n\t* ls - directory listing command. Parameters: ls [dir]; \nif no parameters are specified, the current directory is listed.\
\n\t* logout - the command to disconnect from the server. Has no parameters\
\n\t* chmod - command to set file/directory permissions. \nParameters: chmod [file name] [access value]\nExample: chmod user/home/file1.txt 777\
\n\t* crF - creates a file with the specified name. Parameters: crF [file_name]\
\n\t* crD - creates a directory with the specified name. Parameters: crD [path/dir_name] [rights]\nIf the path is not specified, creates in the current directory.\
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
	shutdown(currentSocket, NULL);
	closesocket(currentSocket);
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

	cout << "\n#################################_Waiting for the first connection._#################################\n";

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
		cout << "\n----------------------Client " << ClientSocket[counter] << " connected----------------------\n";
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

//��� ����������� ����� ��������� ������� ���������� ��� ������� � ����� ������ (�������� �����)
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

	if (login == ERROR_STR)
	{
		send_data(currentSocket, "ERROR! Invalid command arguments!\n");
		return -1;
	}
	
	ind = find_user(login);
	if (user[ind].auth_token == true)
	{
		send_data(currentSocket, "ERROR! This user is already logged in to another thread!\n");
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
	//�������� �� ������ ������� � ��������� ������ ���. ������
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

	//���� ������� � �� ������
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
	case 2:
		ret = authorize(currentSocket);
		return ret;
	case 3:
		ret = rr(currentSocket);
		//ret = send_data(currentSocket, "This command is not supported in the current version of the server! I'm working on it :)\n");
		return ret;
	case 8:
		if (logged_in(currentSocket))
			ret = chmod(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 0:
		ret = help(currentSocket);
		return ret;
	case -11111:
		send_data(currentSocket, "This command is not supported in the current version of the server! I'm working on it :)\n");
		return 0;
	case -1111:
		send_data(currentSocket, "This command is not supported in the current version of the server! I'm working on it :)\n");
		return 0;
	case 9:
		if (logged_in(currentSocket))
			ret = createFile(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 11:
		if (logged_in(currentSocket))
			ret = deleteFD(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 10:
		if (logged_in(currentSocket))
			ret = createDirectory(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 12:
		if (logged_in(currentSocket))
			ret = cd(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 13:
		if (logged_in(currentSocket))
			ret = createGroup(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 14:
		if (logged_in(currentSocket))
			ret = deleteGroup(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 15:
		if (logged_in(currentSocket))
			ret = addUserInGroup(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 16:
		if (logged_in(currentSocket))
			ret = deleteUserInGroup(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 4:
		if (logged_in(currentSocket))
			ret = write(currentSocket); 
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 5:
		if (logged_in(currentSocket))
			ret = read(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 6:
		if (logged_in(currentSocket))
			ret = ls(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 7:
		if (logged_in(currentSocket))
			ret = logout(currentSocket, th_id);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 17:
		if (logged_in(currentSocket))
			ret = createUser(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 18:
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
	case 19:
		if (logged_in(currentSocket))
			ret = changeLUser(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 21:
		if (logged_in(currentSocket))
			ret = listUser(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!");
		return ret;
	case 1:
		ret = reg(currentSocket, USER);
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

inline string Server::get_level_access(int ind, string path, bool INFO_FL)
{
	//���������, ���������� �� ����������, �� ������� ���������� �������� �������� �������
	string temp_path = path;
	while (temp_path.size() != 0)
	{
		if (!fs::exists(temp_path))
		{
			int i = temp_path.size() - 1;
			// �������� �� ���� ��������� ����������
			while (temp_path[i] != '\\')
				i--;
			i--;
			temp_path.clear();
			for (i; i >= 0; i--)
			{
				temp_path.insert(temp_path.begin(), path[i]);
			}
			path = temp_path;
		}
		else
		{
			path = temp_path;
			break;
		}
	}

	pair<string, string> key = make_pair<string, string>(user[ind].login.c_str(), path.c_str());
	string access;

	//���� � ������� ���� ���������� ��� ����������� �����
	if (LMatrix.find(key) != LMatrix.end())
		access = LMatrix.find(key)->second;
	else if (LMatrix.find(make_pair<string, string>(OTHER_GROUP, path.c_str())) != LMatrix.end())
		access = LMatrix.find(make_pair<string, string>(OTHER_GROUP, path.c_str()))->second;
	else
	{
		for (int i = 0; i < user[ind].group.size(); i++)
		{
			if (LMatrix.find(make_pair<string, string>(user[ind].group[i].c_str(), path.c_str())) != LMatrix.end())
			{
				access = LMatrix.find(make_pair<string, string>(user[ind].group[i].c_str(), path.c_str()))->second;
				break;
			}
		}
	}
	if (access.empty())
	{
		cout << "\n\t\t\t\aInternal server error! The access rights value for this key was not found!\n";
		return "0";
	}
	if (INFO_FL)
	{
		string _access = access.erase(3);
		return _access;
	}

	string owner, _gr;
	vector<string> groups;

	//�������� ��������� � ������ ��� ������� ����
	int it = 3;
	while (access[it] != '/')
	{
		owner += access[it];
		it++;
	}
	it++;
	while (access[it] != '\0')
	{
		if (access[it] == '/')
		{
			groups.push_back(_gr);
			_gr.clear();
		}
		_gr += access[it];
		it++;
	}
	groups.push_back(_gr);

	//���� ������ ����� ������� ����������� ����� ����, ��� ������ ���������, �� ������� �������� ������� �������
	int _ind = find_user(owner);
	if (user[ind].status >= user[_ind].status)
		return "7";

	//�������� �������� ����� ������� ��� ����� �� ���� 'path'
	string _access;
	if (user[ind].login == owner)
		_access = access[0];
	else {
		for (int i = 0; i < groups.size(); i++)
		{
			for (int j = 0; j < user[ind].group.size(); j++)
			{
				if (groups[i] == user[ind].group[j])
				{
					_access = access[1];
					break;
				}
			}
		}
	}
	if (_access.empty())
		_access = access[2];
	return _access;
}

inline void Server::set_level_access(int ind, string path, string &access)
{
	LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(user[ind].login.c_str(), path.c_str()), access.c_str()));

	for (int i = 0; i < user[ind].group.size(); i++)
		LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(user[ind].group[i].c_str(), path.c_str()), access.c_str()));

	LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(OTHER_GROUP, path.c_str()), access.c_str()));


	string temp_path = path;
	// �������� �� ���� ��������� ����������
	int i = temp_path.size() - 1;
	while (temp_path[i] != '\\')
		i--;
	i--;
	temp_path.clear();
	for (i; i >= 0; i--)
		temp_path.insert(temp_path.begin(), path[i]);
	path = temp_path;

	//���� � ���� �� ���������� �����-���� ����������, �� ��������� �� � ������� ���� � ���� �� ���������� �������, ��� � ��� �������� ����������
	while (LMatrix.find(make_pair<string, string>(user[ind].login.c_str(), path.c_str())) == LMatrix.end() && path != SERVER_ROOT_PATH)
	{

		//pair<string, string> key = make_pair<string, string>(user[ind].login.c_str(), path.c_str());
		LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(user[ind].login.c_str(), path.c_str()), access.c_str()));

		for (int i = 0; i < user[ind].group.size(); i++)
			LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(user[ind].group[i].c_str(), path.c_str()), access.c_str()));

		LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(OTHER_GROUP, path.c_str()), access.c_str()));

		// ���������� �������� �� ���� ��������� ����������
		int i = temp_path.size() - 1;
		while (temp_path[i] != '\\')
			i--;
		i--;
		temp_path.clear();
		for (i; i >= 0; i--)
			temp_path.insert(temp_path.begin(), path[i]);
		path = temp_path;
	}

	//��������� � ���� ������� ���� ������� 
	fstream out_file(MATRIX_LAW_PATH);

	map<pair<string, string>, string>::iterator it;
	if (!out_file.is_open())
		out_file.open(MATRIX_LAW_PATH);

	for (it = LMatrix.begin(); it != LMatrix.end(); it++)
		out_file << it->first.first << " " << it->first.second << " " << it->second << endl;

	out_file.close();
}

void Server::change_level_access(int ind, string path, string& access)
{
	map<pair<string, string>, string>::iterator it;

	//�������������� �������� ��� �����
	it = LMatrix.find(make_pair<string, string>(user[ind].login.c_str(), path.c_str()));
	LMatrix.erase(it);
	LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(user[ind].login.c_str(), path.c_str()), access.c_str()));

	//�������������� �������� ��� ����� �����
	for (int i = 0; i < user[ind].group.size(); i++)
	{
		it = LMatrix.find(make_pair<string, string>(user[ind].group[i].c_str(), path.c_str()));
		LMatrix.erase(it);
		LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(user[ind].group[i].c_str(), path.c_str()), access.c_str()));
	}

	//�������������� �������� ��� ���� ��������� ������
	it = LMatrix.find(make_pair<string, string>(OTHER_GROUP, path.c_str()));
	LMatrix.erase(it);
	LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(OTHER_GROUP, path.c_str()), access.c_str()));

	//��������� ���������� ������� � ����
	fstream out_file(MATRIX_LAW_PATH);
	if (!out_file.is_open())
		out_file.open(MATRIX_LAW_PATH);

	for (it = LMatrix.begin(); it != LMatrix.end(); it++)
		out_file << it->first.first << " " << it->first.second << " " << it->second << endl;

	out_file.close();
}

void Server::delete_level_access(int ind, string path)
{
	map<pair<string, string>, string>::iterator it;
	vector<string> box;

	box.push_back(path);
	if (!fs::is_empty(path))
	{
		for (auto const& dir_entry : std::filesystem::recursive_directory_iterator{ path })
		{
			box.push_back(dir_entry.path().string());
		}
	}

	for (int i = 0; i < box.size(); i++)
	{
		path = box[i];
		//������� ��������� ��� �����
		it = LMatrix.find(make_pair<string, string>(user[ind].login.c_str(), path.c_str()));
		if (it != LMatrix.end())
			LMatrix.erase(it);
		else {
			cout << "\n\t\t\tERROR! There is no such key in the access rights matrix!" << endl;
			return;
		}

		//������� �������� ��� ����� �����
		for (int i = 0; i < user[ind].group.size(); i++)
		{
			if (i == 0)
				continue;
			it = LMatrix.find(make_pair<string, string>(user[ind].group[i].c_str(), path.c_str()));
			if (it != LMatrix.end())
				LMatrix.erase(it);
			else {
				cout << "\n\t\t\tERROR! There is no such key in the access rights matrix!" << endl;
				return;
			}
		}

		//������� �������� ��� ���� ��������� ������
		it = LMatrix.find(make_pair<string, string>(OTHER_GROUP, path.c_str()));
		if (it != LMatrix.end())
			LMatrix.erase(it);
		else {
			cout << "\n\t\t\tERROR! There is no such key in the access rights matrix!" << endl;
			return;
		}
	}

	//��������� ���������� ������� � ����
	fstream out_file(MATRIX_LAW_PATH);
	if (!out_file.is_open())
		out_file.open(MATRIX_LAW_PATH);

	for (it = LMatrix.begin(); it != LMatrix.end(); it++)
		out_file << it->first.first << " " << it->first.second << " " << it->second << endl;

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
	cout << "SENDING to " << currentSocket << " :: ";

	// -> ���������� ������ ������
	// -> ���������� ��� �����
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
		cout << "successfully" << endl;
	}
}

inline int Server::send_data(SOCKET& currentSocket)
{
	cout << "\tSENDING";

	// -> ���������� ������ ������
	// -> ���������� ��� �����
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

int Server::reg(SOCKET& currentSocket, int initiator)
{
	string log, pass;
	log.reserve(10); 
	pass.reserve(20);

	//�������� �� ������� ������� ����� � ������
	log = get_opt(0, 2);
	pass = get_opt(1, 2);
	//���� ����� �� ���, ������ ����� ������
	if (log == ERROR_STR)
	{
		send_data(currentSocket, "ERROR! Invalid command arguments!\n");
		return -1;
	}
	else if (log == "none")
	{
		send_data(currentSocket, "ERROR! Invalid login!\n");
		return -1;
	}

	//��������� ����� � ������ �� �������
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

	//��������� ����� � ������ �� ������������ �������
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

	//��������� �� ����� �� ���� �����
	if (find_user(log) != -1)
	{
		send_data(currentSocket, "ERROR! A user with this username already exists!\n");
		return -1;
	}


	//�������� ������ ��� ������ �����, 
	// ��������� ��������� � ��������� ����� � ������� ����
	users_count++;
	user = reallocated(users_count);
	
	if (initiator > USER)
	{
		user[users_count - 1].auth_token = false;
		user[users_count - 1].current_path = '\\' + log + "\\home";
		user[users_count - 1].cur_sock = NULL;
		user[users_count - 1].home_path = '\\' + log + "\\home";
		user[users_count - 1].login = log;
		user[users_count - 1].password = pass;
		user[users_count - 1].status = USER;
		user[users_count - 1].group.push_back(user[users_count - 1].login);
	}
	else {
		user[users_count - 1].auth_token = true;
		user[users_count - 1].current_path = '\\' + log + "\\home";
		user[users_count - 1].cur_sock = currentSocket;
		user[users_count - 1].home_path = '\\' + log + "\\home";
		user[users_count - 1].login = log;
		user[users_count - 1].password = pass;
		user[users_count - 1].status = USER;
		user[users_count - 1].group.push_back(user[users_count - 1].login);
	}

	fs::create_directories(SERVER_ROOT_PATH + user[users_count - 1].current_path);
	string access = "700" + user[users_count - 1].login + "/" + user[users_count - 1].login,
		path = SERVER_ROOT_PATH;
	path += user[users_count - 1].current_path;
	set_level_access(users_count - 1, path, access);
	send_data(currentSocket, user[users_count - 1].current_path);

	save_users_data();
	return 0;
}

int Server::chmod(SOCKET& currentSocket)
{
	string opt[2], 
		path = SERVER_ROOT_PATH;
	int ind = find_user(currentSocket);
	opt[0] = get_opt(0, 2);
	opt[1] = get_opt(1, 2);

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
		//���������� � �������� ������� ����� ��������� � ��� ������ ����� ����
		opt[1] += user[ind_owner].login;
		for (int i = 0; i < user[ind_owner].group.size(); i++)
			opt[1] += "/" + user[ind_owner].group[i];

		change_level_access(ind_owner, path, opt[1]);
		send_data(currentSocket, "Successfully!\n#" + user[ind].current_path);
	}
	else
		send_data(currentSocket, "ERROR! You don't have enough rights to execute this command!\n" + user[ind].current_path);
	return 0;
}

string Server::get_owner(string path)
{
	string _owner, owner;
	pair<string, string> key;
	for (int i = 0; i < MAX_LOAD; i++)
	{
		key = make_pair<string, string>(user[i].login.c_str(), path.c_str());
		if (LMatrix.find(key) != LMatrix.end())
		{
			owner = LMatrix.find(key)->second;
			break;
		}
	}
	for (int i = 3; owner[i] != '/'; i++)
		_owner += owner[i];

	return _owner;
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
	string opt = get_opt(0, 1),
		result;
	if (opt == ERROR_STR)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return 0;
	}

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
	string file_name, _access, path = SERVER_ROOT_PATH;

	file_name += get_opt(0, 2);
	_access = get_opt(1, 2);
	int ind = find_user(currentSocket);

	if (file_name[0] == '\\')
		path += file_name;
	else
		path += user[ind].current_path + '\\' + file_name;

	if (file_name == ERROR_STR || _access == ERROR_STR)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n" + user[ind].current_path);
		return 0;
	}
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
	ofstream file;

	if (access == "0" || access == "1" || access == "4" || access == "5")
	{
		send_data(currentSocket, "ERROR! You don't have access rights!\n" + user[ind].current_path);
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
	//else
	//{
	//	switch (static_cast<char>(access[0]))
	//	{
	//	case '7': //������ ������
	//		user[ind].current_path = user[ind].home_path + "\\" + file_name;
	//		break;
	//	case '6': //�� ��������� �������������
	//		user[ind].current_path = user[ind].home_path + "\\" + file_name;
	//		break;
	//	case '3': //��� ������ �� ������� � ����������
	//		break;
	//	case '2': //��� ������� �� ������� � ��������
	//		break;
	//	default:
	//		cout << "\n\t\t\t\aInternal error! Incorrect access rights value !\n";
	//		break;
	//	}
	//}

	//���������� � �������� ������� ����� ��������� � ��� ������ ����� ����
	_access += user[ind].login;
	for (int i = 0; i < user[ind].group.size(); i++)
		_access += "/" + user[ind].group[i];

	set_level_access(ind, path, _access);
	send_data(currentSocket, "Successfull!\n" + user[ind].current_path);

	return 0;
}

int Server::deleteFD(SOCKET& currentSocket)
{
	int ind = find_user(currentSocket);
	string dir_name,
		path = SERVER_ROOT_PATH;

	dir_name.reserve(15);
	dir_name = get_opt(0, 1);
	if (dir_name == ERROR_STR)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n" + user[ind].current_path);
		return 0;
	}
	if (check_path(dir_name, FILE_FL) == INV_SYMBOL_IN_PATH)
	{
		send_data(currentSocket, "ERROR! Invalid symbol in the directory name!\n" + user[ind].current_path);
		return 0;
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
	if (access == "0" || access == "1" || access == "4" || access == "5")
	{
		send_data(currentSocket, "ERROR! You don't have access rights!\n" + user[ind].current_path);
		return 0;
	}

	delete_level_access(ind, path);
	fs::remove_all(path);
	send_data(currentSocket, "Successfull!\n" + user[ind].current_path);

	//��������� ��� ������� ���������� � ������� �� ���������
	/*if (SERVER_ROOT_PATH + '\\' + user[ind].current_path == path)
	{
		for (int i = user[ind].current_path.size(); i >= 0; i--)
		{
			 
		}
	}*/

	return 0;
}

int Server::createDirectory(SOCKET& currentSocket)
{
	int ind = find_user(currentSocket);
	string dir_name, opt_0, opt_1,
		path = SERVER_ROOT_PATH;


	dir_name.reserve(15);
	opt_0 = get_opt(0, 2);
	opt_1 = get_opt(1, 2);
	if (opt_0 == ERROR_STR || opt_1 == ERROR_STR)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n" + user[ind].current_path);
		return 0;
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
	
	//������������� ���������� � ����� ������
	if (fs::exists(path))
	{
		send_data(currentSocket, "ERROR! A directory with this name already exists!\n" + user[ind].current_path);
		return 0;
	}
	//������� ���� �� �������� � ���� ����������
	string access = get_level_access(ind, path, false);

	if (access == "0" || access == "1" || access == "4" || access == "5")
	{
		send_data(currentSocket, "ERROR! You don't have access rights!\n" + user[ind].current_path);
		return 0;		
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
		case '7': //������ ������
			user[ind].current_path = user[ind].home_path + "\\" + dir_name;
			break;
		case '6': //�� ��������� �������������
			user[ind].current_path = user[ind].home_path + "\\" + dir_name;
			break;
		case '3': //��� ������ �� ������� � ����������
			break;
		case '2': //��� ������� �� ������� � ��������
			break;
		default:
			cout << "\n\t\t\t\aInternal error! Incorrect access rights value !\n";
			break;
		}
	}

	//���������� � �������� ������� ����� ��������� � ��� ������ ����� ����
	for (int i = 0; i < user[ind].group.size(); i++)
	{
		if (i == 0)
		{
			opt_1 += user[ind].group[i];
			continue;
		}
		opt_1 += "/" + user[ind].group[i];
	}

	set_level_access(ind, path, opt_1);
	send_data(currentSocket, user[ind].current_path.c_str());

	return 0;
}

int Server::cd(SOCKET& currentSocket)
{
	string opt, path = SERVER_ROOT_PATH;
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
		if (opt == ERROR_STR)
		{
			send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
			return 0;
		}
		if (check_path(opt, DIR_FL) == INV_SYMBOL_IN_PATH)
		{
			send_data(currentSocket, "ERROR! Invalid symbol in the directory name!\n" + user[ind].current_path);
			return 0;
		}
		if (opt[0] == '\\')
			path += opt;
		else
			path += user[ind].current_path + "\\" + opt;

		//��������� ������������� ����� ����������
		if (fs::exists(path) == false)
		{
			send_data(currentSocket, "ERROR! A directory with this name already exists!\n#" + user[ind].current_path);
			return 0;
		}
		//��������� ������ � ���������� � ������� ����
		string access = get_level_access(ind, path, false);
		
		if (access == "4" || access == "7" || access == "6" || access == "5")
		{
			if (opt[0] == '\\')
				user[ind].current_path = "#" + opt;
			else
				user[ind].current_path += "\\" + opt;
			send_data(currentSocket, user[ind].current_path);
		}
		else
			send_data(currentSocket, "ERROR! You don't have access rights to this directory!\n#" + user[ind].current_path);
	}
	return 0;
}

int Server::createGroup(SOCKET& currentSocket)
{
	int ind = find_user(currentSocket);
	if (user[ind].status < ADMIN)
	{
		send_data(currentSocket, "ERROR! Not enough permissions to execute this command!\n#" + user[ind].current_path);
		return 0;
	}
	string opt = get_opt(0, 1);
	if (opt == ERROR_STR)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return 0;
	}

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
	string data, file_name;

	//����� �������� ������ ������� ��������� ���������� ��� ���� �������; ��������� ����������� ��������
	file_name = get_opt(0, 2);
	data = get_opt(1, 2);

	int ind = find_user(currentSocket);
	if (data == ERROR_STR || file_name == ERROR_STR)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return 0;
	}
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
	//��������� ������������� ������ �����
	if (fs::exists(path) == false)
	{
		send_data(currentSocket, "ERROR! A file with this name already exists!\n#" + user[ind].current_path);
		return 0;
	}
	//��������� ������ � ����� � ������� ����
	string access = get_level_access(ind, path, false);
	if (access == "2" || access == "3" || access == "6" || access == "7")
	{
		fstream file;
		file.open(file_name, ios::app);
		if (!file.is_open())
		{
			send_data(currentSocket, "Internal server error! Couldn't open the file " + file_name + "\n#" + user[ind].current_path);
			return 0;
		}
		file << data;
		file.close();
		send_data(currentSocket, "successfull!\n#" + user[ind].current_path);
	}
	else
	{
		send_data(currentSocket, "ERROR! You don't have write access to this file!\n#" + user[ind].current_path);
		return 0;
	}

	return 0;
}

int Server::read(SOCKET& currentSocket)
{
	string data, file_name, tmp;
	file_name = get_opt(0, 1);

	int ind = find_user(currentSocket);
	if (file_name == ERROR_STR)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return 0;
	}
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
	//��������� ������������� ������ �����
	if (fs::exists(path) == false)
	{
		send_data(currentSocket, "ERROR! A file with this name already exists!\n#" + user[ind].current_path);
		return 0;
	}

	//��������� ������ � ����� � ������� ����
	string access = get_level_access(ind, path, false);
	if (access == "2" || access == "3" || access == "6" || access == "7")
	{
		fstream file;
		file.open(file_name, ios::in);
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
		return 0;
	}
	else
	{
		send_data(currentSocket, "ERROR! You don't have read access to this file!\n#" + user[ind].current_path);
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

	path.reserve(50);
	path = SERVER_ROOT_PATH;
	if (cmd_buffer.size() != 0)
	{
		opt = get_opt(0, 1);
		if (opt == ERROR_STR)
		{
			send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
			return 0;
		}

		if (opt[0] == '\\')
			path += opt;
		else
			path += user[ind].current_path + '\\' + opt;
	}
	else
		path += user[ind].current_path;

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

	send_data(currentSocket, out_str + "\n\n#" + user[ind].current_path);

	return 0;
}

int Server::logout(SOCKET& currentSocket, int th_id)
{
	int cl_id = find_user(currentSocket);
	send_data(currentSocket, "disconnected");
	
	//��������� ���������� �����
	close_client(currentSocket, th_id, cl_id);

	//��������� ������ ������
	save_users_data();

	//��������� �������������� ������ � ����
	free_index.push(th_id);
	return 1234;
}

int Server::createUser(SOCKET& currentSocket)
{
	int ind = find_user(currentSocket);
	if (user[ind].status < ADMIN)
	{
		send_data(currentSocket, "ERROR! To execute this command, you must have an ADMIN access level and higher.\n#" + user[ind].current_path);
		return 0;
	}
	reg(currentSocket, user[ind].status);

	return 0;
}

int Server::deleteUser(SOCKET& currentSocket)
{
	int ind = find_user(currentSocket);
	if (user[ind].status < ADMIN)
	{
		send_data(currentSocket, "ERROR! To execute this command, you must have an ADMIN access level and higher.\n#" + user[ind].current_path);
		return 0;
	}

	string login = get_opt(0, 1);
	if (login == ERROR_STR)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return 0;
	}
	int del_ind = find_user(login);
	if (del_ind == -1)
	{
		send_data(currentSocket, "ERROR! There is no user with this username!\n#" + user[ind].current_path);
		return 0;
	}
	


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

	string login = get_opt(0, 2),
		pass = get_opt(1, 2);

	if (login == ERROR_STR || pass == ERROR_STR)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n#" + user[ind].current_path);
		return 0;
	}
	int user_ind = find_user(login);
	if (user_ind == -1)
	{
		send_data(currentSocket, "ERROR! There is no user with this username!\n#" + user[ind].current_path);
		return 0;
	}

	//change password
	user[user_ind].password = pass;

	return 0;

} 

int Server::changeLUser(SOCKET& currentSocket)
{
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
		result += "\nLOGIN: " + user[i].login;
		if (user[i].status == USER)
			result += "\nSTATUS: user";
		else if (user[i].status == ADMIN)
			result += "\nSTATUS: admin";
		if (user[i].auth_token)
			result += "\nAUTHORIZE: authorize";
		else
			result += "\nAUTHORIZE: not authorize ";

		result += "\nCURR. SOCKET: " +  to_string(user[i].cur_sock);
		result += "\nUSERS GROUPS : ";
		for (int j = 0; j < user[i].group.size(); j++)
		{
			result += user[ind].group[j] + " ";
		}
		result += "\n-------------------\n";
	}

	send_data(currentSocket, result + "\n#" + user[ind].current_path);
	return 0;
}

int Server::loadMatrixLaw()
{
	fstream law;
	string key1, key2, value;
	law.open(MATRIX_LAW_PATH);

	if (!law.is_open())
	{
		cout << "\n\t\tERROR OPEN FILE 'LawMatrix.txt'!\n";
		return 0;
	}
	while (law.eof() != true)
	{
		law >> key1 >> key2 >> value;
		LMatrix.insert( make_pair<pair<string, string>, string> (make_pair<string, string>(key1.c_str(), key2.c_str()), value.c_str()));
	}
	return 1;
}

string Server::get_opt(int n, int opt_count)
{
	string opt;
	opt.reserve(100);
	vector<string> box;
	int numb_opt = n;

	for (int i = 0; i < cmd_buffer.size() +1; i++)
	{
		if (cmd_buffer[i] == ' ' || cmd_buffer[i] == '\0')
		{
			box.push_back(opt);
			if (opt.empty())
				return ERROR_STR;
			opt.clear();
			continue;
		}
		opt += cmd_buffer[i];
	}
	if (box.size() != opt_count)
		return ERROR_STR;

	return box[n];
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

int Server::check_path(string& path, int fl)
{
	if (fl == DIR_FL)
	{
		for (int i = 0; i < path.size(); i++)
		{
			if (path[i] == 34 || path[i] == 42 || path[i] == 46 || path[i] == 47 || path[i] == 58 || path[i] == 60 || path[i] == 62 || path[i] == 63 || path[i] == 124)
				return INV_SYMBOL_IN_PATH;
		}
	}
	else if (fl == FILE_FL)
	{
		for (int i = 0; i < path.size(); i++)
		{
			if (path[i] == 34 || path[i] == 42 || path[i] == 47 || path[i] == 58 || path[i] == 60 || path[i] == 62 || path[i] == 63 || path[i] == 124)
				return INV_SYMBOL_IN_PATH;
		}
	}

	return -10;
}

int Server::rr(SOCKET& currentSocket)
{
	int ind = find_user(currentSocket);
	string fname = get_opt(0, 1), 
		path = SERVER_ROOT_PATH;

	if (fname == ERROR_STR)
	{
		send_data(currentSocket, "ERROR! Incorrect command options!\n" + user[ind].current_path);
		return 0;
	}
	if (check_path(fname, FILE_FL) == INV_SYMBOL_IN_PATH)
	{
		send_data(currentSocket, "ERROR! Invalid symbol in the directory name!\n" + user[ind].current_path);
		return 0;
	}

	if (fname[0] == '\\')
		path += fname;
	else
		path += user[ind].current_path + '\\' + fname;

	//������������� ���������� � ����� ������
	if (!fs::exists(path))
	{
		send_data(currentSocket, "ERROR! There is no directory/file on this path!\n" + user[ind].current_path);
		return 0;
	}

	string access = get_level_access(ind, path, true);
	
	send_data(currentSocket, "Access rights for this path: " + access + "\n#" + user[ind].current_path);
	return 0;
}