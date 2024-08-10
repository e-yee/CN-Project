#include "stdafx.h"
#include "Server2.h"
#include <afxsock.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinApp theApp;

using namespace std;
using std::cout;

CSocket sServer;

DWORD WINAPI uploadProcess(LPVOID arg) {
	SOCKET* h_connected = (SOCKET*) arg;
	CSocket sConnector;
	sConnector.Attach(*h_connected);

	string filename = "download.txt";
	vector<File> downloadable_list;
	
	//Send downloadable files to Client
	getDownloadableFiles(downloadable_list, filename);
	sendDownloadableFiles(downloadable_list, sConnector);

	int start = 0;
	bool connection = true;
	vector<File> requesting_list;
	vector<queue<Header>> file_headers_list;

	//Receive requesting files from Client
	receiveRequestingFiles(requesting_list, sConnector, start, connection);
	getFileHeadersList(file_headers_list, requesting_list, downloadable_list);
	
	//Check connection
	if (!connection) {
		delete h_connected;
		return 0;
	}

	vector<int> list_of_size;

	//Send list of file size to Client
	getListOfFileSize(list_of_size, requesting_list, start);
	sendListOfFileSize(list_of_size, sConnector, start);

	int number_of_chunks = 0;
	int chunk_count = 0;
	int difference = 0;
	int finish = requesting_list.size();
	Header head;
	vector<ifstream> ifs_list;

	//Send data to Client
	do {
		for (int i = 0; i < requesting_list.size(); ++i) {
			if (file_headers_list[i].empty()) continue;

			getNumberOfChunks(requesting_list[i].priority, number_of_chunks, file_headers_list[i].size());
			sConnector.Send(&number_of_chunks, sizeof(int), 0);

			while (number_of_chunks != 0) {
				head = file_headers_list[i].front();
				file_headers_list[i].pop();
				sendHeader(head, sConnector);

				//Handle chunk position
				if (head.position == "start") {
					ifstream ifs(head.filename.c_str(), ios::binary);
					ifs_list.push_back(move(ifs));
				}
				else if (head.position == "end") {
					--finish;
					++chunk_count;
					--number_of_chunks;
					ifs_list[i].close();

					continue;
				}

				sendChunk(head, ifs_list[i], sConnector, connection);

				//Check connection
				if (!connection) {
					delete h_connected;
					return 0;
				}

				++chunk_count;
				--number_of_chunks;

				//Receive difference after every two chunks sent
				sConnector.Receive((char*)&difference, sizeof(int), 0);

				//Update requesting list
				if (difference == 1) {
					receiveRequestingFiles(requesting_list, sConnector, start, connection);
					getFileHeadersList(file_headers_list, requesting_list, downloadable_list);

					//Check connection
					if (!connection) {
						delete h_connected;
						return 0;
					}

					finish += requesting_list.size() - start;

					getListOfFileSize(list_of_size, requesting_list, start);
					sendListOfFileSize(list_of_size, sConnector, start);
				}
			}
		}

		if (finish == 0) break;

		if (downloadable_list.empty()) break;
	} while (1);

	delete h_connected;
	return 0;
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[]) {
	int n_ret_code = 0;

	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0)) {
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		n_ret_code = 1;
	}
	else {
		if (AfxSocketInit() == FALSE) {
			cout << "Socket Library initialization failed\n";
			return FALSE;
		}

		if (sServer.Create(1234, SOCK_STREAM, NULL) == 0) {
			cout << "Server initialization failed\n";
			cout << sServer.GetLastError();
			return FALSE;
		}
		else {
			cout << "Server initialization succeeded\n";

			if (sServer.Listen(1) == FALSE) {
				cout << "Server cannot listen on this port\n";
				sServer.Close();
				return FALSE;
			}
		}

		DWORD thread_ID;
		HANDLE thread_status;
		CSocket sConnector;

		cout << "Waiting connection from Client\n";
		while (1) {
			if (sServer.Accept(sConnector)) {
				cout << "Client connected to Server\n";

				SOCKET* h_connected = new SOCKET();

				*h_connected = sConnector.Detach();

				thread_status = CreateThread(NULL, 0, uploadProcess, h_connected, 0, &thread_ID);
			}
		}
		sServer.Close();
	}

	return n_ret_code;
}


