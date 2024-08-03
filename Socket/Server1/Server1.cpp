#include "stdafx.h"
#include "Server1.h"
#include <vector>
#include <afxsock.h>
#include "Function.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinApp theApp;
CSocket server_socket;

using namespace std;

void signalHandler(int signum) {
	const char* termination_message = "404 - Server disconnection";
	int message_size = static_cast<int>(strlen(termination_message));

	if (server_socket.m_hSocket != INVALID_SOCKET) {
		server_socket.Send(&message_size, sizeof(message_size), 0);
		server_socket.Send(termination_message, message_size, 0);
		server_socket.Close();
	}
	exit(signum);
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
			cout << "Socket Library initialization failed\n";
			return FALSE;
		}

		if (server_socket.Create(1234, SOCK_STREAM, NULL) == 0) {
			cout << "Server initialization failed\n";
			cout << server_socket.GetLastError();
			return FALSE;
		}
		else {
			cout << "Server intialization succeeded\n";

			if (server_socket.Listen(1) == FALSE) {
				cout << "Server cannot listen on this port\n";
				server_socket.Close();
				return FALSE;
			}
		}

		while (1) {
			CSocket connector;
			cout << "Waiting connection from Client!\n";

			if (server_socket.Accept(connector)) {
				cout << "Client connected to Server!\n";

				vector<File> file_list;
				sendFileList(file_list, connector);

				uploadProcess(file_list, connector);
			}
			connector.Close();
		}
		server_socket.Close();
	}

	return n_ret_code;
}
