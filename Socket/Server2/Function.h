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
	int chunk_size;
};

bool checkDisconnection(int error_code);
void getDownloadableFiles(vector<File>& downloadable_list, string filename);
void sendDownloadableFiles(vector<File> downloadable_list, CSocket& sConnector);
void receiveRequestingFiles(vector<File>& requesting_list, CSocket& sConnector, int& start, bool& connection);
void getListOfFileSize(vector<int>& list_of_size, vector<File> requesting_list, int start);
void sendListOfFileSize(vector<int> list_of_size, CSocket& sConnector, int start);
void getFileHeaders(queue<Header>& file_headers, string fname);
void getFileHeadersList(vector<queue<Header>>& file_headers_list, vector<File> requesting_list, vector<File>& downloadable_list);
void sendHeader(Header head, CSocket& sConnector);
void sendChunk(Header head, ifstream& ifs, CSocket& sConnector, bool& connection);
void getNumberOfChunks(string priority, int& number_of_chunks, int file_headers_size);