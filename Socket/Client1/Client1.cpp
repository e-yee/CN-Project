#include "stdafx.h"
#include "Client1.h"
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
			cout << "Server connection succeeded\n";

			char ClientMsg[100];
			int MsgSize;
			char* temp;

			do {
				ClientSocket.Receive((char*)&MsgSize, sizeof(int), 0);
				temp = new char[MsgSize + 1];
				ClientSocket.Receive((char*)temp, MsgSize, 0);

				temp[MsgSize] = '\0';
				cout << "Server: " << temp << "\n";

				cout << "Client: ";
				cin.getline(ClientMsg, 100);

				MsgSize = strlen(ClientMsg);

				ClientSocket.Send(&MsgSize, sizeof(MsgSize), 0);

				ClientSocket.Send(ClientMsg, MsgSize, 0);
				delete[] temp;
			} while (1);
		}
		else cout << "Server connection failed\n";
		
		ClientSocket.Close();
	}

	return nRetCode;
}