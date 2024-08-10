#pragma once
#include "stdafx.h"
#include <afxsock.h>

using namespace std;

struct File {
	string name;
	string size;
	string priority;
};

struct Header {
	string filename;
	string position;
};

void receiveDownloadableFiles(CSocket& sClient);
void getRequestingFiles(vector<File>& requesting_list, string filename);
void sendRequestingFiles(vector<File> requesting_list, CSocket& sClient, int start);
void receiveListOfFileSize(vector<int>& list_of_size, CSocket& sClient);
void receiveHeader(Header& head, CSocket& sClient);
void receiveChunk(ofstream& ofs, CSocket& sClient, int chunk_size, int& bytes);