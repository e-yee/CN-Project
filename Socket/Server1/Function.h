#pragma once
#include "stdafx.h"
#include <afxsock.h>

using namespace std;

struct File {
	string name, size;
};

vector<File> getFileList(string file_name);
void sendFileList(vector<File>& file_list, CSocket& connector);
bool checkConnection(int error_code);
void uploadFile(string file_name, CSocket& connector, int& connection);
void uploadProcess(vector<File>& file_list, CSocket& connector);