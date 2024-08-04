#include "stdafx.h"
#include "Server2.h"
#include <afxsock.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinApp theApp;

CSocket server_socket;
CSocket connector;
vector<File> file_list;
queue<File> thread_requesting_list;
int difference = -1;

using namespace std;
using std::cout;

void signalHandler(int signum) {
	int termination_message = 404;

	if (server_socket.m_hSocket != INVALID_SOCKET) {
		server_socket.Send(&termination_message, sizeof(int), 0);
		server_socket.Close();
	}
	exit(signum);
}

DWORD WINAPI updateRequestingList(LPVOID arg) {
	SOCKET* h_connected = (SOCKET*)arg;
	CSocket my_sock;
	my_sock.Attach(*h_connected);

	difference = 0;

	int message_size = 0;
	char* message;

	do {
		connector.Receive((char*)&message_size, sizeof(int));

		if (checkDisconnection(message_size)) break;

		message = new char[message_size + 1];
		connector.Receive(message, message_size);
		message[message_size] = '\0';

		queue<File> requesting_list;
		getRequestingList(requesting_list, message);

		if (!thread_requesting_list.empty()) {
			difference = 1;
			
			thread_requesting_list = requesting_list;
		}
		else {
			if (requesting_list.back().name != thread_requesting_list.back().name) {
				difference = 1;
				
				while (requesting_list.front().name != thread_requesting_list.front().name)
					requesting_list.pop();

				while (!thread_requesting_list.empty()) {
					thread_requesting_list.pop();
					requesting_list.pop();
				}	

				thread_requesting_list = requesting_list;
			}
		}

		Sleep(2000);
	} while (1);

	delete h_connected;
	return 0;
}

DWORD WINAPI uploadProcess(LPVOID arg) {
	SOCKET* k_connected = (SOCKET*)arg;
	CSocket my_sock;
	my_sock.Attach(*k_connected);

	queue<File> requesting_list = thread_requesting_list;
	queue<DataChunk> chunk_queue;
	queue<queue<DataChunk>> buffer_queue;
	queue<int> file_size_list;
	int file_size = -1;

	while (!requesting_list.empty()) {
		for (int i = 0; i < file_list.size(); ++i)
			if (requesting_list.front().name == file_list[i].name) {
				getChunkQueue(chunk_queue, file_list[i], file_size);

				buffer_queue.push(chunk_queue);
				file_size_list.push(file_size);

				requesting_list.pop();
				file_list.erase(file_list.begin() + i);
			}
	}

	int response = -1;
	sendData(buffer_queue, connector, response);
	if (response == 0) {
		delete k_connected; 
		return 0;
	}

	delete k_connected;
	return 0;
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[]) {
	signal(SIGINT, signalHandler);

	int n_ret_code = 0;

	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0)) {
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		n_ret_code = 1;
	}
	else {
		if (AfxSocketInit() == FALSE) {
			cout << "Socket Library intialization failed\n";
			return FALSE;
		}

		if (server_socket.Create(1234, SOCK_STREAM, NULL) == 0) {
			cout << "Server initialization failed\n";
			cout << server_socket.GetLastError();
			return FALSE;
		}
		else {
			cout << "Server initialization succeeded\n";

			if (server_socket.Listen(1) == FALSE) {
				cout << "Server cannot listen on this port\n";
				server_socket.Close();
				return FALSE;
			}
		}

		DWORD thread_ID1;
		HANDLE thread_status1;

		cout << "Waiting connection from Client\n";
		while (1) {
			if (server_socket.Accept(connector)) {
				cout << "Client connected to Server\n";

				getFileList(file_list);
				sendFileList(file_list, connector);

				SOCKET* h_connected = new SOCKET();
				*h_connected = connector.Detach();
				thread_status1 = CreateThread(NULL, 0, updateRequestingList, h_connected, 0, &thread_ID1);

				if (difference == 1) {
					DWORD thread_ID2;
					HANDLE thread_status2;
					SOCKET* k_connected = new SOCKET();
					*k_connected = connector.Detach();
					thread_status2 = CreateThread(NULL, 0, uploadProcess, k_connected, 0, &thread_ID2);
				}
			}
		}
		server_socket.Close();
	}

	return n_ret_code;
}


