#include "Function.h"

bool checkDisconnection(int bytes_received) {
	if (bytes_received == 0) {
		cout << "Client disconnected from Server!\n";
		return true;
	}
	return false;
}

void getFileList(vector<File>& file_list) {
	string file_name = "download.txt";
	ifstream ifs(file_name.c_str());
	if (!ifs.good()) {
		cout << "Open file failed!\n";
		return;
	}

	File f;
	while (!ifs.eof()) {
		ifs >> f.name >> f.size;
		
		file_list.push_back(f);
	}

	ifs.close();
}

void sendFileList(vector<File> file_list, CSocket& connector) {
	string message = "";
	for (int i = 0; i < file_list.size(); ++i) {
		if (i != file_list.size() - 1)
			message += file_list[i].name + " " + file_list[i].size + "\n";
		else
			message += file_list[i].name + " " + file_list[i].size;
	}
	int message_size = message.size();

	connector.Send(&message_size, sizeof(message_size));
	connector.Send(message.c_str(), message_size);
}

void getRequestingList(queue<File>& requesting_list, string message) {
	stringstream ss(message);
	File f;
	while (ss.good()) {
		ss >> f.name >> f.priority;

		requesting_list.push(f);
	}
}

void getChunkQueue(queue<DataChunk>& chunk_queue, File requesting_file, int& file_size) {
	ifstream ifs(requesting_file.name.c_str(), ios::binary);
	if (!ifs.good()) {
		cout << "Open file failed!\n";
		return;
	}

	DataChunk chunk;
	chunk.file_name = requesting_file.name;

	int max_chunk_size = 0;
	if (requesting_file.priority == "NORMAL") max_chunk_size = 6400;
	else if (requesting_file.priority == "HIGH") max_chunk_size = 6400 * 4;
	else max_chunk_size = 6400 * 10;

	ifs.seekg(0, ios::end);
	file_size = ifs.tellg();
	ifs.seekg(0, ios::beg);

	int number_of_chunks = file_size / max_chunk_size;
	int rest_data = file_size % max_chunk_size;
	char* buffer;
	if (number_of_chunks == 0) {
		buffer = new char[rest_data];

		ifs.read(buffer, rest_data);

		chunk.position = "end";
		chunk.size = to_string(rest_data);
		chunk.content = buffer;

		chunk_queue.push(chunk);
	}
	else {
		buffer = new char[max_chunk_size];

		for (int i = 0; i < number_of_chunks; ++i) {
			ifs.read(buffer, max_chunk_size);

			if (i == 0) 
				chunk.position = "start";
			else if (i == number_of_chunks - 1) 
				chunk.position = "end";
			else 
				chunk.position = "middle";
			chunk.size = to_string(max_chunk_size);
			chunk.content = buffer;

			chunk_queue.push(chunk);
		}

		if (rest_data != 0) {
			chunk_queue.back().position = "middle";

			ifs.read(buffer, rest_data);

			chunk.position = "end";
			chunk.size = to_string(rest_data);
			chunk.content = buffer;

			chunk_queue.push(chunk);
		}
	}
}

void sendData(queue<queue<DataChunk>>& buffer_queue, CSocket& connector, int& response) {
	DataChunk data;
	queue<DataChunk> chunk_queue;
	int bytes_received = 0;
	while (!buffer_queue.empty()) {
		chunk_queue = buffer_queue.front();
		buffer_queue.pop();

		data = chunk_queue.front();
		chunk_queue.pop();

		connector.Send((char*)&data, sizeof(data));
		connector.Receive((char*)&bytes_received, sizeof(int));

		if (checkDisconnection(bytes_received)) {
			response = 0;
			return;
		}

		if (!chunk_queue.empty()) buffer_queue.push(chunk_queue);
	}
}