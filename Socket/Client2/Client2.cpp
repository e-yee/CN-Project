#include "stdafx.h"
#include "Client2.h"
#include <afxsock.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinApp theApp;

using namespace std;

CSocket sClient;
vector<File> thread_requesting_list;
string input = "input.txt";
int difference = 0;
int start = 0;

DWORD WINAPI updateRequestingList(LPVOID arg) {
	vector<File> new_requesting_list;
	int new_size = 0;
	int old_size = 0;

	while (1) {
		getRequestingFiles(new_requesting_list, input);

		if (thread_requesting_list.size() != new_requesting_list.size()) {
			difference = 1;

			new_size = new_requesting_list.size();
			old_size = thread_requesting_list.size();
			for (int i = old_size; i < new_size; ++i)
				thread_requesting_list.push_back(new_requesting_list[i]);	
			
			start = old_size;
		}

		Sleep(2000);
	}

	return 0;
}

void signalHandler(int signum) {
	if (sClient.m_hSocket != INVALID_SOCKET) {
		int termination_code = -3;
		sClient.Send(&termination_code, sizeof(int), 0);
		sClient.Close();
	}

	exit(signum);
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[]) {
	signal(SIGINT, signalHandler);

	int nRetCode = 0;

	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0)) {
		_tprintf(_T("<<< Fatal Error: MFC initialization failed >>>\n"));
		nRetCode = 1;
		return nRetCode;
	}
	if (AfxSocketInit() == FALSE) {
		cout << "<<< Fatal Error: Socket Library initialization failed >>>\n";
		return FALSE;
	}

	sClient.Create();
	if (sClient.Connect(_T("192.168.1.10"), 1234) == 0) {
		cout << "\r<<< Server connection failed... Trying again...>>>\n";
		return nRetCode;
	}
	cout << "<<< Server connection succeeded >>>\n\n\n";

	receiveDownloadableFiles(sClient);

	while (1) {
		getRequestingFiles(thread_requesting_list, input);
		if (!thread_requesting_list.empty()) break;
	}
	sendRequestingFiles(thread_requesting_list, sClient, 0);

	vector<int> progress;
	for (int i = 0; i < thread_requesting_list.size(); ++i)
		progress.push_back(0);
	
	DWORD thread_ID;
	HANDLE thread_status;
	thread_status = CreateThread(NULL, 0, updateRequestingList, NULL, 0, &thread_ID);
	
	vector<int> list_of_size;
	receiveListOfFileSize(list_of_size, sClient);

	int number_of_chunks = 0;
	Header head;
	int file_position = -1;
	vector<ofstream> ofs_list;
	int rest_data = 0;
	int bytes_received = 0;
	int chunk_count = 0;
	int chunk_size = 0;
	int finish = thread_requesting_list.size();
	do {
		sClient.Receive((char*)&number_of_chunks, sizeof(int), 0);

		while (number_of_chunks != 0) {
			receiveHeader(head, sClient);

			//Get file position
			for (int i = 0; i < thread_requesting_list.size(); ++i) 
				if (head.filename == thread_requesting_list[i].name) {
					file_position = i;
					break;
				}

			//Handle chunk position
			if (head.position == "start") {
				string path = "C:\\Users\\PC\\Desktop\\" + head.filename;
				ofstream ofs(path.c_str(), ios::app | ios::binary);
				ofs_list.push_back(move(ofs));
			}
			else if (head.position == "end") {
				--finish;
				++chunk_count;
				--number_of_chunks;
				ofs_list[file_position].close();

				continue;
			}

			//Determine chunk size
			rest_data = list_of_size[file_position] - progress[file_position];
			if (rest_data < 10240) chunk_size = rest_data;
			else chunk_size = 10240;

			//Receive data and update progress
			bytes_received = 0;
			receiveChunk(ofs_list[file_position], sClient, chunk_size, bytes_received);
			progress[file_position] += bytes_received;

			++chunk_count;
			--number_of_chunks;

			//Send difference after every chunk sent
			sClient.Send(&difference, sizeof(int), 0);

			//Update requesting list
			if (difference == 1) {
				difference = 0;

				sendRequestingFiles(thread_requesting_list, sClient, start);
				for (int i = start; i < thread_requesting_list.size(); ++i)
					progress.push_back(0);

				finish += thread_requesting_list.size() - start;

				receiveListOfFileSize(list_of_size, sClient);
			}

			//cout << 1.0 * progress[file_position] / list_of_size[file_position] * 100 << "\n";
			displayProgress(progress, list_of_size, thread_requesting_list);
		}

		if (finish == 0) break;
	} while (1);
	
	return nRetCode;
}