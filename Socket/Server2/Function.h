#pragma once
#include "stdafx.h"
#include <afxsock.h>

using namespace std;

struct File {
	string name, size, priority;
};

struct Chunk {
	string file_name, IP_address, position, content;
};

vector<File> getFileList(string file_name);
void sendFileList(vector<File>& file_list, CSocket& connector);
void uploadFile(string file_name, CSocket& connector);
void uploadProcess(vector<File>& file_list, CSocket& connector);