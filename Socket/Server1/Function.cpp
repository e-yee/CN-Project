#include "Function.h"

vector<File> getFileList(string file_name) {
	vector<File> v;

	ifstream ifs(file_name.c_str());
	if (!ifs.is_open()) {
		cout << "Open file failed!\n";
		return v;
	}

	File f;
	while (!ifs.eof()) {
		getline(ifs, f.name, ' ');
		getline(ifs, f.size);

		v.push_back(f);
	}

	ifs.close();

	return v;
}

void sendFileList(vector<File>& file_list, CSocket& connector) {
	string file_name = "Resource Files\\download.txt";
	file_list = getFileList(file_name);

	string list = file_list[0].name + " " + file_list[0].size + "\n";
	for (int i = 1; i < file_list.size(); i++) {
		if (i < file_list.size() - 1)
			list += file_list[i].name + " " + file_list[i].size + "\n";
		else
			list += file_list[i].name + " " + file_list[i].size;
	}
	int list_size = list.size();

	connector.Send(&list_size, sizeof(int), 0);
	connector.Send(list.c_str(), list_size, 0);
}

bool checkConnection(int error_code) {
	if (error_code == -3 || error_code == 0) {
		cout << "Client disconnected from Server\n";
		return false;
	}

	return true;
}

void uploadFile(string file_name, CSocket& connector, int& connection) {
	ifstream ifs(file_name.c_str(), ios::binary);
	if (!ifs.is_open()) {
		cout << "Open file failed!\n";
		return;
	}

	ifs.seekg(0, ios::end);
	int real_size = ifs.tellg();
	ifs.seekg(0, ios::beg);

	connector.Send(&real_size, sizeof(int), 0);

	int bytes_sent = 0;
	int bytes_received = 0;
	int bytes_offset = 0;
	int chunk_size = 10240;
	char chunk[10240] = {};

	while (real_size >= 10240) {
		ifs.read(chunk, chunk_size);

		bytes_sent = chunk_size;
		connector.Send(chunk, bytes_sent, 0);
		connector.Receive((char*)&bytes_received, sizeof(int), 0);
		
		if (!checkConnection(bytes_received)) {
			connection = 0;
			ifs.close();
			return;
		}

		//Resend if Client cannot receive data chunk
		while (bytes_received == -1)
			connector.Receive((char*)&bytes_received, sizeof(int), 0);

		//Send the rest until Client receives full data chunk
		while (bytes_received < bytes_sent) {
			bytes_sent -= bytes_received;
			connector.Receive((char*)&bytes_received, sizeof(int), 0);

			if (!checkConnection(bytes_received)) {
				connection = 0;
				ifs.close();
				return;
			}

			//Resend if Client cannot receive data chunk
			while (bytes_received == -1)
				connector.Receive((char*)&bytes_received, sizeof(int), 0);
		}
		real_size -= chunk_size;
	}

	bytes_sent = bytes_received = 0;
	char* rest_chunk;
	while (real_size > 0) {
		rest_chunk = new char[real_size];

		bytes_offset = bytes_sent - bytes_received;
		ifs.seekg(-bytes_offset, ios::cur);
		ifs.read(rest_chunk, real_size);

		bytes_sent = real_size;
		connector.Send(rest_chunk, bytes_sent, 0);
		connector.Receive((char*)&bytes_received, sizeof(int), 0);

		if (!checkConnection(bytes_received)) {
			connection = 0;
			delete[] rest_chunk;
			ifs.close();
			return;
		}

		//Resend if Client cannot receive data chunk
		while (bytes_received == -1) 
			connector.Receive((char*)&bytes_received, sizeof(int), 0);

		real_size -= bytes_received;
		delete[] rest_chunk;
	}

	ifs.close();
}

void uploadProcess(vector<File>& file_list, CSocket& connector) {
	int response = -1;
	int message_size = 0;
	int connection = 1;
	char* message;

	do {
		connector.Receive((char*)&message_size, sizeof(int), 0);

		if (!checkConnection(message_size)) return;

		message = new char[message_size + 1];
		connector.Receive(message, message_size, 0);
		message[message_size] = '\0';

		response = -1;
		for (int i = 0; i < file_list.size(); ++i)
			if (strcmp(file_list[i].name.c_str(), message) == 0) {
				response = 1;

				uploadFile(file_list[i].name, connector, connection);
				if (connection == 0) return;

				file_list.erase(file_list.begin() + i);
				break;
			}

		if (file_list.empty()) {
			response = 0;

			connector.Send(&response, sizeof(int), 0);
			cout << "Client has downloaded all files\n";
			break;
		}

		connector.Send(&response, sizeof(int), 0);
	} while (1);
}