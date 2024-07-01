#include "stdafx.h"
#include "Server.h"
#include <afxsock.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinApp theApp;

using namespace std;

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[]) {
	int nRetCode = 0;

	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0)) {
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		nRetCode = 1;
	}
	else {
		if (AfxSocketInit() == FALSE) {
			cout << "Socket Library initialization failed\n";
			return FALSE;	
		}
		
		CSocket ServerSocket;

		if (ServerSocket.Create(1234, SOCK_STREAM, NULL) == 0) {
			cout << "Server creation failed!\n";
			cout << ServerSocket.GetLastError();
			return FALSE;
		}
		else {
			cout << "Server creation succeeded!\n";

			if (ServerSocket.Listen(1) == FALSE) {
				cout << "Cannot listen on this port!\n";
				ServerSocket.Close();
				return FALSE;
			}
		}

		CSocket Connector;

		if (ServerSocket.Accept(Connector)) {
			cout << "Client connected!\n";

			char ServerMsg[100];
			int MsgSize;
			char* temp;

			do {
				cout << "Server: ";
				cin.getline(ServerMsg, 100);

				MsgSize = strlen(ServerMsg);

				Connector.Send(&MsgSize, sizeof(MsgSize), 0);

				Connector.Send(ServerMsg, MsgSize, 0);

				Connector.Receive((char*)&MsgSize, sizeof(int), 0);
				temp = new char[MsgSize + 1];
				Connector.Receive((char*)temp, MsgSize, 0);

				temp[MsgSize] = '\0';
				cout << "Client: " << temp << "\n";
				delete temp;
			} while (1);
		}
		Connector.Close();
		ServerSocket.Close();
	}

	return nRetCode;
}