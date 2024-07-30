#include "stdafx.h"
#include "Server2.h"
#include <afxsock.h>

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

DWORD WINAPI functionCal(LPVOID arg)
{
	SOCKET* hConnected = (SOCKET*)arg;
	CSocket mysock;
	//Chuyen ve lai CSocket
	mysock.Attach(*hConnected);

	int number_continue = 0;

	do {
		fflush(stdin);
		int number_a, number_b, number_result;
		char letter;
		//Nhan phep toan
		mysock.Receive(&letter, sizeof(letter), 0);
		//Nhan so thu nhat
		mysock.Receive(&number_a, sizeof(number_a), 0);
		//Nhan so thu hai
		mysock.Receive(&number_b, sizeof(number_b), 0);

		//So sanh neu client muon thuc hien phep cong
		if (letter == '+')
			number_result = number_a + number_b;
		else if (letter == '-')
			number_result = number_a - number_b;

		//Gui ket qua tinh toan cho client
		mysock.Send(&number_result, sizeof(number_result), 0);

		//Nhan number xem client co tiep tuc hay khong
		mysock.Receive(&number_continue, sizeof(number_continue), 0);

	} while (number_continue);
	delete hConnected;
	return 0;
	//return 0;
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

		CSocket connector;
		DWORD thread_ID;
		HANDLE thread_status;
		
		cout << "Waiting connection from Client\n";
		while (1) {
			if (server_socket.Accept(connector)) {
				cout << "Client connected to Server\n";

				SOCKET* h_connected = new SOCKET();

				*h_connected = connector.Detach();

				thread_status = CreateThread(NULL, 0, functionCal, h_connected, 0, &thread_ID);
			}
		}
		server_socket.Close();
	}

	return n_ret_code;
	//HMODULE h_module = ::GetModuleHandle(NULL);

	//if (h_module != NULL)
	//{
	//	// initialize MFC and print and error on failure
	//	if (!AfxWinInit(h_module, NULL, ::GetCommandLine(), 0)) {
	//		// TODO: change error code to suit your needs
	//		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
	//		n_ret_code = 1;
	//	}
	//	else {
	//		// TODO: code your application's behavior here.
	//		AfxSocketInit(NULL);
	//		CSocket connector;
	//		DWORD threadID;
	//		HANDLE threadStatus;

	//		server.Create(4567);
	//		do {
	//			printf("Server lang nghe ket noi tu client\n");
	//			server.Listen();
	//			server.Accept(connector);
	//			//Khoi tao con tro Socket
	//			SOCKET* hConnected = new SOCKET();
	//			//Chuyển đỏi CSocket thanh Socket
	//			*hConnected = connector.Detach();
	//			//Khoi tao thread tuong ung voi moi client Connect vao server.
	//			//Nhu vay moi client se doc lap nhau, khong phai cho doi tung client xu ly rieng
	//			threadStatus = CreateThread(NULL, 0, function_cal, hConnected, 0, &threadID);
	//		} while (1);
	//	}
	//}
	//else
	//{
	//	// TODO: change error code to suit your needs
	//	_tprintf(_T("Fatal Error: GetModuleHandle failed\n"));
	//	n_ret_code = 1;
	//}

	//return n_ret_code;
}


