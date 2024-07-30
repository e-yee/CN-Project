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

bool checkDisconnection(int bytes_received, string IP_address) {
	if (bytes_received == 0) {
		cout << "Client " << IP_address << " disconnected from Server!\n";

		return true;
	}
	return false;
}

vector<File> getRequestingList(string message) {
	stringstream ss(message);
	File f;
	vector<File> result;

	while (ss.good()) {
		getline(ss, f.name, ' ');
		getline(ss, f.priority);

		result.push_back(f);
	}

	return result;
}

void uploadFile(File f, string IP_address) {

}
void uploadProcess(vector<File>& file_list, CSocket& connector, string IP_address) {
	int response;
	int message_size;
	int bytes_received;
	char* message;

	do {
		bytes_received = connector.Receive(&message_size, sizeof(int), 0);
		if (checkDisconnection(bytes_received, IP_address)) break;

		message = new char[message_size + 1];
		connector.Receive(message, message_size, 0);
		message[message_size] = '\0';

		vector<File> requesting_list = getRequestingList(message);
	} while (1);
}