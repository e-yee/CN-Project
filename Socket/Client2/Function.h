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

void displayProgress(const vector<int> current_progress, const vector<int> total_progress, const vector<File> files);
void receiveDownloadableFiles(CSocket& sClient);
void getRequestingFiles(vector<File>& requesting_list, string filename);
void sendRequestingFiles(vector<File> requesting_list, CSocket& sClient, int start);
void receiveListOfFileSize(vector<int>& file_size_list, CSocket& sClient);
void receiveHeader(Header& head, CSocket& sClient);
void receiveChunk(ofstream& ofs, CSocket& sClient, int chunk_size, int& bytes);