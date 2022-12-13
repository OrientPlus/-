#include "Server.h"

//_________________________________________________________
//          получение и парсинг команды от клиента        |
//---------------------------------------------------------

int Server::get_command(SOCKET currentSocket)
{
	int ind = find_user(currentSocket);

	err_val1 = recv(currentSocket, (char*)&recvBuf_size, sizeof(int), NULL);
	recvBuffer = new char[recvBuf_size];
	err_val2 = recv(currentSocket, recvBuffer, recvBuf_size, NULL);
	if (ind != -1 && user[ind].del_tocken)
	{
		delete[] recvBuffer;
		return EXIT;
	}
	if (err_val1 == SOCKET_ERROR || err_val2 == SOCKET_ERROR)
	{
		cout << "Getting failed! :: " << currentSocket << endl;
		shutdown(currentSocket, 2);
		closesocket(currentSocket);
		err_val1 = WSAGetLastError();
		cout << "WSA ERROR IN 'get_comand' FUNC --> " << err_val1 << endl;
	}
	else if (err_val2 < recvBuf_size)
	{
		cout << "\n\t\tWAITING ALL DATA\n\n";
		system("pause");
	}

	cout << recvBuffer << " :: " << currentSocket << endl;
	cmd_buffer = recvBuffer;
	delete[] recvBuffer;
	//============================================================================
	return 0;
}

int Server::pars_command(SOCKET& currentSocket, int th_id)
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
	for (int i = 0; i < 25; i++)
	{
		if (command == supported_commands[i])
		{
			switch_on = i;
			break;
		}
		else if (i == 24)
		{
			command.clear();
			cmd_buffer.clear();
			return INV_CMD;
		}
	}

	switch (switch_on)
	{
	case 24:
		if (logged_in(currentSocket))
			ret = write(currentSocket, true);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 2:
		ret = authorize(currentSocket, th_id);
		return ret;
	case 3:
		ret = rr(currentSocket);
		return ret;
	case 8:
		if (logged_in(currentSocket))
			ret = chmod(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 0:
		ret = help(currentSocket);
		return ret;
	case 22:
		if (logged_in(currentSocket))
			ret = cm(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 23:
		if (logged_in(currentSocket))
			ret = chM(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 9:
		if (logged_in(currentSocket))
			ret = createFile(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 11:
		if (logged_in(currentSocket))
			ret = deleteFD(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 10:
		if (logged_in(currentSocket))
			ret = createDirectory(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 12:
		if (logged_in(currentSocket))
			ret = cd(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 13:
		if (logged_in(currentSocket))
			ret = createGroup(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 14:
		if (logged_in(currentSocket))
			ret = deleteGroup(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 15:
		if (logged_in(currentSocket))
			ret = addUserInGroup(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 16:
		if (logged_in(currentSocket))
			ret = deleteUserInGroup(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 4:
		if (logged_in(currentSocket))
			ret = write(currentSocket, false);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 5:
		if (logged_in(currentSocket))
			ret = read(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 6:
		if (logged_in(currentSocket))
			ret = ls(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 7:
		if (logged_in(currentSocket))
			ret = logout(currentSocket, th_id);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 17:
		if (logged_in(currentSocket))
			ret = createUser(currentSocket, th_id);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 18:
		if (logged_in(currentSocket))
			ret = deleteUser(currentSocket, th_id);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 20:
		if (logged_in(currentSocket))
			ret = changePUser(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 19:
		if (logged_in(currentSocket))
			ret = changeLUser(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 21:
		if (logged_in(currentSocket))
			ret = listUser(currentSocket);
		else
			send_data(currentSocket, "To execute this command, you need to log in!\n");
		return ret;
	case 1:
		ret = reg(currentSocket, USER, th_id);
		return ret;
	default:
		return INV_CMD;
	}
}

//________________________________________________________
//               работа с матрицей прав                  |
//--------------------------------------------------------

string Server::get_level_access(int ind, string path, bool INFO_FL)
{
	//проверяем, существует ли директория, по которой необходимо получить значение доступа
	string temp_path = path;
	while (temp_path.size() != 0)
	{
		if (!fs::exists(temp_path))
		{
			int i = temp_path.size() - 1;
			// вырезаем из пути последнюю директорию
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

	//ищем в матрице ключ подходящий для конкретного юзера
	if (LMatrix.find(key) != LMatrix.end())
		access = LMatrix.find(key)->second;
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
		access = LMatrix.find(make_pair<string, string>(OTHER_GROUP, path.c_str()))->second;

	if (access.empty())
	{
		cout << "\n\t\t\t\aInternal server error! The access rights value for this key was not found!\n";
		return "0";
	}
	if (INFO_FL)
		return access;

	string owner, _gr;
	vector<string> groups;

	//выделяем владельца и группы для данного пути
	int it = 3;
	while (access[it] != '/' && access[it] != '\0')
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
			it++;
			continue;
		}
		_gr += access[it];
		it++;
	}
	groups.push_back(_gr);

	//если статус юзера который запрашивает права выше, чем статус владельца, то вернуть значение полного доступа
	int _ind = find_user(owner);
	if (user[ind].status > user[_ind].status)
		return "7";

	//выделяем значение права доступа для юзера на путь 'path'
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

void Server::set_level_access(int ind, string path, string& access, int mark)
{
	//добавляем объект в мандатную систему
	ms.set_mark(path, mark);

	LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(user[ind].login.c_str(), path.c_str()), access.c_str()));

	for (int i = 0; i < user[ind].group.size(); i++)
		LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(user[ind].group[i].c_str(), path.c_str()), access.c_str()));

	LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(OTHER_GROUP, path.c_str()), access.c_str()));


	string temp_path = path;
	// вырезаем из пути последнюю директорию
	int i = temp_path.size() - 1;
	while (temp_path[i] != '\\')
		i--;
	i--;
	temp_path.clear();
	for (i; i >= 0; i--)
		temp_path.insert(temp_path.begin(), path[i]);
	path = temp_path;

	//если в пути не существует какой-либо директории, то добавляем ее в матрицу прав с теми же значениями доступа, что и для конечной директории
	while (LMatrix.find(make_pair<string, string>(user[ind].login.c_str(), path.c_str())) == LMatrix.end() && path != SERVER_ROOT_PATH)
	{
		//==========DELETE THIS CODE AFTER DEBUGGING!==========
		for (int i = 0; i < user[ind].group.size(); i++)
		{
			if (user[ind].group[i].size() < 4)
			{
				cout << "\n\a\tFIND INCORRECT GROUP!" << endl;
				system("pause");
			}
		}
		//=====================================================

		//добавляем объект в мандатную систему
		ms.set_mark(path, mark);

		LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(user[ind].login.c_str(), path.c_str()), access.c_str()));

		for (int i = 0; i < user[ind].group.size(); i++)
			LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(user[ind].group[i].c_str(), path.c_str()), access.c_str()));

		LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(OTHER_GROUP, path.c_str()), access.c_str()));

		// аналогично вырезаем из пути последнюю директорию
		int i = temp_path.size() - 1;
		while (temp_path[i] != '\\')
			i--;
		i--;
		temp_path.clear();
		for (i; i >= 0; i--)
			temp_path.insert(temp_path.begin(), path[i]);
		path = temp_path;
	}

	//сохраняем в файл матрицу прав доступа 
	save_Matrix_Law();
}

void Server::add_level_access(int ind, string group)
{
	map<pair<string, string>, string>::iterator it;
	vector<string> box;

	string init = SERVER_ROOT_PATH, access;
	init += '\\' + user[ind].login;
	box.push_back(init);
	for (auto const& dir_entry : std::filesystem::recursive_directory_iterator{ init })
		box.push_back(dir_entry.path().string());

	for (int i = 0; i < box.size(); i++)
	{
		init = box[i];
		access = get_level_access(ind, init, true);
		access += "/" + group;
		LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(group.c_str(), init.c_str()), access.c_str()));
	}

	//сохраняем в файл матрицу прав доступа 
	save_Matrix_Law();

	return;
}

void Server::change_level_access(int ind, string path, string& access)
{
	map<pair<string, string>, string>::iterator it;

	//перезаписываем значение для юзера
	it = LMatrix.find(make_pair<string, string>(user[ind].login.c_str(), path.c_str()));
	LMatrix.erase(it);
	LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(user[ind].login.c_str(), path.c_str()), access.c_str()));

	//перезаписываем значение для групп юзера
	for (int i = 0; i < user[ind].group.size(); i++)
	{
		it = LMatrix.find(make_pair<string, string>(user[ind].group[i].c_str(), path.c_str()));
		LMatrix.erase(it);
		LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(user[ind].group[i].c_str(), path.c_str()), access.c_str()));
	}

	//перезаписываем значения для всех остальных юзеров
	it = LMatrix.find(make_pair<string, string>(OTHER_GROUP, path.c_str()));
	LMatrix.erase(it);
	LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(OTHER_GROUP, path.c_str()), access.c_str()));

	//сохраняем измененную матрицу в файл
	save_Matrix_Law();
}

void Server::delete_level_access(int ind, string path, int FL)
{
	map<pair<string, string>, string>::iterator it;
	vector<string> box;

	box.push_back(path);
	if (!fs::is_empty(path) && FL == DIR_FL)
	{
		for (auto const& dir_entry : std::filesystem::recursive_directory_iterator{ path })
		{
			box.push_back(dir_entry.path().string());
		}
	}

	for (int i = 0; i < box.size(); i++)
	{
		path = box[i];
		//удаляем значение в мандатной системе
		ms.delete_marks(path);

		//удаляем значениие для юзера
		it = LMatrix.find(make_pair<string, string>(user[ind].login.c_str(), path.c_str()));
		if (it != LMatrix.end())
			LMatrix.erase(it);
		else {
			cout << "\n\t\t\tERROR! There is no such key in the access rights matrix!" << endl;
			return;
		}

		//удаляем значение для групп юзера
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

		//удаляем значения для всех остальных юзеров
		it = LMatrix.find(make_pair<string, string>(OTHER_GROUP, path.c_str()));
		if (it != LMatrix.end())
			LMatrix.erase(it);
		else {
			cout << "\n\t\t\tERROR! There is no such key in the access rights matrix!" << endl;
			return;
		}
	}

	//сохраняем измененную матрицу в файл
	save_Matrix_Law();
}

void Server::update_level_access(int ind)
{
	string newValue,
		oldValue;

	for (int j = 0; j < user[ind].group.size(); j++)
	{
		if (j != 0)
			newValue += "/" + user[ind].group[j];
		else
			newValue += user[ind].group[j];
	}

	map<pair<string, string>, string>::iterator it = LMatrix.begin();
	for (it; it != LMatrix.end(); it++)
	{
		if (get_owner(it->first.second) == user[ind].login)
		{
			oldValue = get_level_access(ind, it->first.second, true);
			oldValue.erase(3);
			it->second = oldValue + newValue;
		}
	}
	save_Matrix_Law();
}


//_______________________________________________________
//          логика работы с мандатной системой          |
//-------------------------------------------------------
int Server::MANDSYS::load_marks()
{
	fstream in;
	in.open(MAND_SYS_PATH, ios::in);
	if (!in.is_open())
	{
		cout << "\n\aCant open 'mandSys.txt' file! The structure of the mandate system is not initialized!\n";
		//throw - 1;
		return -1;
	}
	
	string key;
	int value;
	while (!in.eof())
	{
		in >> key >> value;
		marks.insert(make_pair(key, value));
	}

	in.close();
	return 0;
}

int Server::MANDSYS::save_marks()
{
	fstream out;
	out.open(MAND_SYS_PATH, ios::out);
	if (!out.is_open())
	{
		cout << "\n\aCant open 'mandSys.txt' file!\nATTENTION!!! The structure of the mandate system has not been preserved! \n\
		The next time the server is started, the system will not be initialized!\n";
		return -1;
	}
	map<string, int>::iterator it = marks.begin();
	for (it; it != marks.end(); it++)
		out << it->first << " " << it->second << endl;

	return 0;
}

int Server::MANDSYS::get_mark(string key)
{
	string temp_path = key, path = key;
	while (temp_path.size() != 0)
	{
		if (!fs::exists(temp_path))
		{
			int i = temp_path.size() - 1;
			// вырезаем из пути последнюю директорию
			while (temp_path[i] != '\\')
			{
				if (i == 0)
					break;
				i--;
			}
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
	if (path.size() != 0)
		key = path;
	map<string, int>::iterator it = marks.find(key);

	if (it == marks.end())
		return -1;
	else
		return it->second;
}

int Server::MANDSYS::set_mark(string key, int value)
{
	marks.insert(make_pair(key, value));
	save_marks();
	return 0;
}

int Server::MANDSYS::change_mark(string key, int value)
{
	map<string, int>::iterator it = marks.find(key);

	if (it == marks.end())
	{
		cout << "\n\aWARNING! No mark found for the key '" << key << "'!" << endl;
		return -1;
	}
	else
	{
		it->second = value;
		return 0;
	}
}

int Server::MANDSYS::delete_marks(string key)
{
	map<string, int>::iterator it = marks.find(key);

	if (it != end(marks))
	{
		marks.erase(it);
		save_marks();
		return 0;
	}
	else
	{
		cout << "\n\t\aInternal server error! There is no key: '" << key << "' in the mandatory system!" << endl;
		return -1;
	}
}


vector<string> Server::get_opt(bool AUTH_FL)
{
	string opt;
	opt.reserve(100);

	vector<string> box;
	int it = 0;

	if (AUTH_FL)
	{
		it = cmd_buffer.find(" ");
		opt = cmd_buffer;
		opt.erase(it);
		box.push_back(opt);
		opt.clear();
		opt.reserve(cmd_buffer.size() - it);
		it++;
		for (int i = 0; i < cmd_buffer.size() - it; i++)
			opt.push_back(cmd_buffer[i + it]);

		box.push_back(opt);

		return box;
	}

	while (!cmd_buffer.empty())
	{
		it = cmd_buffer.find(" ");
		if (it == -1)
			it = cmd_buffer.size();

		opt = cmd_buffer;
		opt.erase(it);
		box.push_back(opt);
		opt.clear();
		if (it == cmd_buffer.size())
			break;

		it++;
		opt.resize(cmd_buffer.size() - it);
		for (int i = 0; i < cmd_buffer.size() - it; i++)
			opt[i] = cmd_buffer[i + it];
		cmd_buffer = opt;
	}
	return box;
}

string Server::get_owner(string path)
{
	string _owner, owner;
	pair<string, string> key;
	for (int i = 0; i < users_count; i++)
	{
		key = make_pair<string, string>(user[i].login.c_str(), path.c_str());
		if (LMatrix.find(key) != LMatrix.end())
		{
			owner = LMatrix.find(key)->second;
			break;
		}
	}
	for (int i = 3; owner[i] != '/'; i++)
	{
		if (owner[i] == '\0')
			break;
		_owner += owner[i];
	}

	return _owner;
}

//  Перевыделение памяти для структуры юзеров
Server::USERS* Server::reallocated(int value)
{
	int it = 0;
	int size = users_count + value;
	USERS* new_user = new USERS[size];
	if (value > 0)
	{
		it = 1;
		users_count += 1;
	}
	else
		users_count -= 1;

	for (int i = 0; i < size - it; i++)
	{
		new_user[i].login = user[i].login;
		new_user[i].password = user[i].password;
		new_user[i].current_path = user[i].current_path;
		new_user[i].home_path = user[i].home_path;
		new_user[i].auth_token = user[i].auth_token;
		new_user[i].status = user[i].status;
		new_user[i].del_tocken = user[i].del_tocken;
		new_user[i].def_mark = user[i].def_mark;
		new_user[i].current_mark = user[i].current_mark;
		if (new_user[i].auth_token)
			new_user[i].cur_sock = user[i].cur_sock;
		for (int j = 0; j < user[i].group.size(); j++)
			new_user[i].group.push_back(user[i].group[j]);
	}

	delete[] Server::user;

	return &new_user[0];
}

//______________________________________________________
//        Сохранение и загрузка данных сервера         |
//------------------------------------------------------
int Server::load_users_data()
{
	//шифрование файла ??? OpenSSL ???
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
		if (i > users_count)
		{
			users_count++;
			user = reallocated(1);
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

		file >> user[i].password;
		file >> user[i].status;
		file >> user[i].def_mark;
		user[i].del_tocken = false;

		i++;
	}
	file.close();

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
		LMatrix.insert(make_pair<pair<string, string>, string>(make_pair<string, string>(key1.c_str(), key2.c_str()), value.c_str()));
	}
	return 1;
}

int Server::save_users_data()
{
	ofstream file;

	file.open(USERS_DATA_PATH);

	if (file.bad() || !file.is_open())
	{
		cout << "\n\t\t\tUnable to open the file::usersData.txt  A new file will be created!\n";
		file.close();

		string name = "usersData_BAD.txt";
		file.open(name);
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
		file << user[i].def_mark << endl;
	}
	file.close();
	return 0;
}

void Server::save_Matrix_Law()
{
	fstream out_file;
	out_file.open(MATRIX_LAW_PATH, ios::out);

	map<pair<string, string>, string>::iterator it;
	if (!out_file.is_open())
	{
		out_file.open(MATRIX_LAW_PATH, ios::trunc);
		cout << "\nLAW MATRIX FILE NOT OPEN!" << endl;
	}

	for (it = LMatrix.begin(); it != LMatrix.end(); it++)
		out_file << it->first.first << " " << it->first.second << " " << it->second << endl;

	out_file.close();
}


int Server::find_user(string& login)
{
	for (int i = 0; i < users_count; i++)
	{
		if (login == user[i].login)
			return i;
	}
	return -1;
}

int Server::find_user(int curSock)
{
	for (int i = 0; i < users_count; i++)
	{
		if (user[i].cur_sock == curSock)
			return i;
	}
	return -1;
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

bool Server::logged_in(int currentSocket)
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

int Server::send_data(SOCKET& currentSocket, string cmd)
{
	cout << "SENDING to " << currentSocket << " :: ";

	// -> отправляем размер буфера
	// -> отправляем сам буфер
	buf_size = cmd.size();
	cmd[buf_size] = '\0';
	buf_size++;

	err_val1 = send(currentSocket, (char*)&buf_size, sizeof(int), NULL);
	err_val2 = send(currentSocket, cmd.c_str(), buf_size, NULL);

	if (err_val1 == SOCKET_ERROR || err_val2 == SOCKET_ERROR)
	{
		cout << "Send failed!" << endl;
		shutdown(currentSocket, 2);
		closesocket(currentSocket);
		return 0;
	}
	else
		cout << cmd << " :: " << currentSocket << endl;

	return 0;
}

int Server::send_data(SOCKET& currentSocket)
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