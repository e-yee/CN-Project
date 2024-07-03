#include "stdafx.h"
#include "Client.h"
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

		CSocket ClientSocket;
		ClientSocket.Create();

		if (ClientSocket.Connect(_T("127.0.0.1"), 1234) != 0) {
			cout << "Server connection succeeded!\n";

			char ClientMsg[100];
			int MsgSize;
			char* ServerMsg;

			do {
				ClientSocket.Receive((char*)&MsgSize, sizeof(int), 0);
				ServerMsg = new char[MsgSize + 1];
				ClientSocket.Receive((char*)ServerMsg, MsgSize, 0);

				ServerMsg[MsgSize] = '\0';
				cout << "Server: " << ServerMsg << "\n";

				delete[] ServerMsg;

				cout << "Client: ";
				cin.getline(ClientMsg, 100);

				MsgSize = strlen(ClientMsg);

				ClientSocket.Send((char*)&MsgSize, sizeof(int), 0);

				ClientSocket.Send(ClientMsg, MsgSize, 0);
				
				if (strcmp(ClientMsg, "quit") == 0) {
					ClientSocket.Close();

					return nRetCode;
				}

			} while (1);
		}
		else cout << "Server connection failed!\n";

		ClientSocket.Close();
	}

	return nRetCode;
}