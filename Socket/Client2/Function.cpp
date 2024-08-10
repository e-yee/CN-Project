#include "Function.h"

void receiveDownloadableFiles(CSocket& sClient) {
	int message_size = -1;
	sClient.Receive((char*)&message_size, sizeof(int), 0);

	char* message = new char[message_size + 1];
	sClient.Receive(message, message_size, 0);
	message[message_size] = '\0';

	cout << "List of downloadable files:\n";
	cout << message << "\n";
}

void getRequestingFiles(vector<File>& requesting_list, string filename) {
	ifstream ifs(filename.c_str());
	if (!ifs.good()) {
		cout << "Fail to open " << filename << "\n";
		return;
	}

	File f;
	while (!ifs.eof()) {
		getline(ifs, f.name, ' ');
		getline(ifs, f.priority);

		if (requesting_list.empty()) {
			if (f.name != "" && f.priority != "")
				requesting_list.push_back(f);
		}
		else {
			int existed = 0;
			for (int i = 0; i < requesting_list.size(); ++i)
				if (f.name == requesting_list[i].name) {
					existed = 1;
					break;
				}

			if (existed == 0) requesting_list.push_back(f);
		}
	}

	ifs.close();
}

void sendRequestingFiles(vector<File> requesting_list, CSocket& sClient, int start) {
	sClient.Send(&start, sizeof(int), 0);

	string message = "";
	for (int i = start; i < requesting_list.size(); ++i) {
		if (i != requesting_list.size() - 1)
			message += requesting_list[i].name + " " + requesting_list[i].priority + "\n";
		else
			message += requesting_list[i].name + " " + requesting_list[i].priority;
	}
	int message_size = message.size();

	sClient.Send(&message_size, sizeof(int), 0);
	sClient.Send(message.c_str(), message_size, 0);
}
	
void receiveListOfFileSize(vector<int>& list_of_size, CSocket& sClient) {
	int number_of_files = 0;
	sClient.Receive((char*)&number_of_files, sizeof(int), 0);

	int file_size = 0;
	for (int i = 0; i < number_of_files; ++i) {
		sClient.Receive((char*)&file_size, sizeof(int), 0);

		list_of_size.push_back(file_size);
	}
}

void receiveHeader(Header& head, CSocket& sClient) {
	int filename_length = 0;
	sClient.Receive((char*)&filename_length, sizeof(int), 0);

	char* filename = new char[filename_length + 1];
	sClient.Receive(filename, filename_length, 0);
	filename[filename_length] = '\0';
	head.filename = filename;

	int position_length = 0;
	sClient.Receive((char*)&position_length, sizeof(int), 0);

	char* position = new char[position_length + 1];
	sClient.Receive(position, position_length, 0);
	position[position_length] = '\0';
	head.position = position;
	
	delete[] filename;
	delete[] position;
}

void receiveChunk(ofstream& ofs, CSocket& sClient, int chunk_size, int& bytes) {
	char* buffer = new char[chunk_size];

	int bytes_received = sClient.Receive(buffer, chunk_size, 0);
	sClient.Send(&bytes_received, sizeof(int), 0);
	ofs.write(buffer, bytes_received);
	bytes += bytes_received;

	while (bytes_received < chunk_size) {
		chunk_size -= bytes_received;

		bytes_received = sClient.Receive(buffer, chunk_size, 0);
		sClient.Send(&bytes_received, sizeof(int), 0);
		ofs.write(buffer, bytes_received);
		bytes += bytes_received;
	}

	delete[] buffer;
}