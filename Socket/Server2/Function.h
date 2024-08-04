#pragma once
#include "stdafx.h"
#include <afxsock.h>

using namespace std;

struct File {
	string name;
	string size;
	string priority;
};

struct DataChunk {
	string file_name;
	string position;
	string size;
	string content;
};

void signalHandler(int signum);
bool checkDisconnection(int error_code);
void getFileList(vector<File>& file_list);
void sendFileList(vector<File> file_list, CSocket& connector);
void getRequestingList(queue<File>& requesting_list, string message);
void getChunkQueue(queue<DataChunk>& q, File f, int& file_size);
void sendData(queue<queue<DataChunk>>& buffer_queue, CSocket& connector, int& response);