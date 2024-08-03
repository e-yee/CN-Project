// Demo_Client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Client2.h"
#include "afxsock.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(NULL);

	if (hModule != NULL)
	{
		// initialize MFC and print and error on failure
		if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
		{
			// TODO: change error code to suit your needs
			_tprintf(_T("Fatal Error: MFC initialization failed\n"));
			nRetCode = 1;
		}
		else
		{
			// TODO: code your application's behavior here.
			AfxSocketInit(NULL);
			CSocket client;

			client.Create();
			client.Connect(_T("127.0.0.1"), 1234);

			int number_continue = 0;
			char letter;
			do {
				fflush(stdin);
				int number_a, number_b, number_result;
				char letter;
				cout << "Nhap vao phep toan (+, -): ";
				cin >> letter;

				cout << "Nhap vao so thu nhat: ";
				cin >> number_a;

				cout << "Nhap vao so thu hai: ";
				cin >> number_b;

				//Gui phep toan den server
				client.Send(&letter, sizeof(letter), 0);
				//Gui so thu nhat den server
				client.Send(&number_a, sizeof(number_a), 0);
				//Gui so thu hai den server
				client.Send(&number_b, sizeof(number_b), 0);

				//Nhan ket qua tinh toan tu server
				client.Receive(&number_result, sizeof(number_result), 0);
				cout << "Ket qua phep toan " << number_a << " " << letter << " " << number_b << " = " << number_result << "\n";

				cout << "Nhap 1 de tiep tuc, 0 de thoat: ";
				cin >> number_continue;
				client.Send(&number_continue, sizeof(number_continue), 0);
			} while (number_continue);

		}
	}
	else
	{
		// TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: GetModuleHandle failed\n"));
		nRetCode = 1;
	}

	return nRetCode;
}
