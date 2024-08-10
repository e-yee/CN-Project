#include "Function.h"

bool checkDisconnection(int error_code) {
	if (error_code == 0 || error_code == -3) {
		cout << "Client disconnected from Server!\n";
		return true;
	}
	
	return false;
}

void getDownloadableFiles(vector<File>& downloadable_list, string filename) {
	ifstream ifs(filename.c_str());
	if (!ifs.good()) {
		cout << "Fail to open " << filename << "!\n";
		return;
	}

	File f;
	while (!ifs.eof()) {
		getline(ifs, f.name, ' ');
		getline(ifs, f.size);

		downloadable_list.push_back(f);
	}

	ifs.close();
}

void sendDownloadableFiles(vector<File> downloadable_list, CSocket& sConnector) {
	string message = "";
	for (int i = 0; i < downloadable_list.size(); ++i) {
		if (i != downloadable_list.size() - 1)
			message += downloadable_list[i].name + " " + downloadable_list[i].size + "\n";
		else
			message += downloadable_list[i].name + " " + downloadable_list[i].size;
	}
	int message_size = message.size();

	sConnector.Send(&message_size, sizeof(int), 0);
	sConnector.Send(message.c_str(), message_size, 0);
}

void receiveRequestingFiles(vector<File>& requesting_list, CSocket& sConnector, int& start, bool& connection) {
	sConnector.Receive((char*)&start, sizeof(int), 0);

	int message_size = 0;
	sConnector.Receive((char*)&message_size, sizeof(int), 0);
	if (checkDisconnection(message_size)) {
		connection = false;
		return;
	}
	
	char* message = new char[message_size + 1];
	sConnector.Receive(message, message_size, 0);
	message[message_size] = '\0';

	stringstream ss(message);
	File f;
	while (ss.good()) {
		getline(ss, f.name, ' ');
		getline(ss, f.priority);

		requesting_list.push_back(f);
	}
}

void getListOfFileSize(vector<int>& list_of_size, vector<File> requesting_list, int start) {
	ifstream ifs;
	int file_size = 0;
	for (int i = start; i < requesting_list.size(); ++i) {
		ifs.open(requesting_list[i].name, ios::binary);

		ifs.seekg(0, ios::end);
		file_size = ifs.tellg();
		ifs.seekg(0, ios::beg);

		ifs.close();

		list_of_size.push_back(file_size);
	}
}

void sendListOfFileSize(vector<int> list_of_size, CSocket& sConnector, int start) {
	int number_of_files = list_of_size.size() - start;
	sConnector.Send(&number_of_files, sizeof(int), 0);

	int file_size = 0;
	for (int i = start; i < list_of_size.size(); ++i) {
		file_size = list_of_size[i];

		sConnector.Send(&file_size, sizeof(int), 0);
	}
}

void getFileHeaders(queue<Header>& file_headers, string fname) {
	ifstream ifs(fname.c_str(), ios::binary);
	if (!ifs.good()) {
		cout << "Fail to open " << fname << "!\n";
		return;
	}

	ifs.seekg(0, ios::end);
	int file_size = ifs.tellg();
	ifs.seekg(0, ios::beg);

	int max_chunk_size = 10240;
	int number_of_chunks = file_size / max_chunk_size;
	int rest_data = file_size % max_chunk_size;
	Header head;

	head.filename = fname;
	for (int i = 0; i < number_of_chunks; ++i) {
		if (i == 0) head.position = "start";
		else head.position = "middle";
		head.chunk_size = max_chunk_size;
		file_headers.push(head);
	}

	if (rest_data != 0) {
		if (file_headers.empty()) {
			head.position = "start";
			head.chunk_size = rest_data;
		}
		else {
			head.position = "middle";
			head.chunk_size = rest_data;
		}
		file_headers.push(head);
	}

	head.position = "end";
	head.chunk_size = 0;
	file_headers.push(head);

	ifs.close();
}

void getFileHeadersList(vector<queue<Header>>& file_headers_list, vector<File> requesting_list, vector<File>& downloadable_list) {
	for (int i = 0; i < requesting_list.size(); ++i) 
		for (int j = 0; j <	downloadable_list.size(); ++j) 
			if (requesting_list[i].name == downloadable_list[j].name) {
				queue<Header> file_headers;
				getFileHeaders(file_headers, requesting_list[i].name);
				file_headers_list.push_back(file_headers);

				downloadable_list.erase(downloadable_list.begin() + j);
			}
}

void sendHeader(Header head, CSocket& sConnector) {
	int filename_length = head.filename.size();
	sConnector.Send(&filename_length, sizeof(int), 0);
	sConnector.Send(head.filename.c_str(), filename_length, 0);

	int position_length = head.position.size();
	sConnector.Send(&position_length, sizeof(int), 0);
	sConnector.Send(head.position.c_str(), position_length, 0);
}

void sendChunk(Header head, ifstream& ifs, CSocket& sConnector, bool& connection) {
	char* buffer = new char[head.chunk_size];
	ifs.read(buffer, head.chunk_size);

	int bytes_sent = head.chunk_size;
	sConnector.Send(buffer, bytes_sent, 0);

	int bytes_received = 0;
	sConnector.Receive((char*)&bytes_received, sizeof(int), 0);

	if (checkDisconnection(bytes_received)) {
		delete[] buffer;
		connection = false;
		return;
	}

	while (bytes_received < bytes_sent) {
		bytes_sent -= bytes_received;
		sConnector.Receive((char*)&bytes_received, sizeof(int), 0);

		if (checkDisconnection(bytes_received)) {
			delete[] buffer;
			connection = false;
			return;
		}
	}

	delete[] buffer;
}

void getNumberOfChunks(string priority, int& number_of_chunks, int file_headers_size) {
	if (priority == "NORMAL") number_of_chunks = 1;
	else if (priority == "HIGH") number_of_chunks = 4;
	else number_of_chunks = 10;

	number_of_chunks = file_headers_size < number_of_chunks ? file_headers_size : number_of_chunks;
}