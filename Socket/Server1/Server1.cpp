#include "stdafx.h"
#include "Server1.h"
#include <afxsock.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinApp theApp;

using namespace std;

struct File {
	string name, size;
};

vector<File> getFileList(string file_name) {
	vector<File> v;

	ifstream ifs(file_name.c_str());
	if (!ifs.is_open()) {
		cout << "Open file failed!\n";
		return v;
	}

	File f;
	while (!ifs.eof()) {
		getline(ifs, f.name, ' ');
		getline(ifs, f.size);

		v.push_back(f);
	}

	ifs.close();

	return v;
}

void uploadProcess(string file_name, CSocket &connector) {
	ifstream ifs(file_name.c_str(), ios::binary);
	if (!ifs.is_open()) {
		cout << "Open file failed!\n";	
		return;
	}

	cout << "Process starts\n";
	ifs.seekg(0, ios::end);
	int r_size = ifs.tellg();
	ifs.seekg(0, ios::beg);

	connector.Send(&r_size, sizeof(int), 0);
	cout << r_size << "\n";

	int chunk_size = 10240;
	char chunk[10240];
	int error = 0;
	bool first_time = true;

	while (r_size >= 10240) {
		if (!first_time) {
			connector.Receive((char*)&error, sizeof(int), 0);

			if (error != 10240) {
				cout << "Resend\n";
				_flushall();
				ifs.seekg(-10240, ios::cur);
				
				ifs.read(chunk, chunk_size);

				connector.Send(&chunk_size, sizeof(int), 0);
				connector.Send(chunk, chunk_size, 0);

				continue;
			}
		}

		ifs.read(chunk, chunk_size);

		connector.Send(&chunk_size, sizeof(int), 0);
		connector.Send(chunk, chunk_size, 0);

		r_size -= 10240;
		first_time = false;
	}

	if (r_size > 0) {
		chunk_size = r_size;
		char* ptr_chunk = new char[chunk_size];

		cout << chunk_size << "\n";
		ifs.read(chunk, chunk_size);

		connector.Send(&chunk_size, sizeof(int), 0);
		connector.Send(ptr_chunk, chunk_size, 0);

		delete[] ptr_chunk;
	}

	ifs.close();
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

		CSocket server_socket;

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

				//Sending list of files to client
				string file_name = "Resource Files\\download.txt";
				vector<File> file_list = getFileList(file_name);

				string list = file_list[0].name + " " + file_list[0].size + "\n";
				for (int i = 1; i < file_list.size(); i++) {
					if (i < file_list.size() - 1)
						list += file_list[i].name + " " + file_list[i].size + "\n";
					else
						list += file_list[i].name + " " + file_list[i].size;
				}
				int list_size = list.size();

				connector.Send(&list_size, sizeof(int), 0);
				connector.Send(list.c_str(), list_size, 0);

				//Uploading process
				int msg_size;
				char* msg;
				do {
					connector.Receive((char*)&msg_size, sizeof(int), 0);
					msg = new char[msg_size + 1];
					connector.Receive(msg, msg_size, 0);
					msg[msg_size] = '\0';

					for (int i = 0; i < file_list.size(); i++) {
						if (strcmp(file_list[i].name.c_str(), msg) == 0) {
							uploadProcess(file_list[i].name, connector);
							cout << "Uploading completed\n";
							file_list.erase(file_list.begin() + i);
							break;
						}
					}

					//Breaking when server catch ctrl-c from client
					if (strcmp(msg, "1111 - Disconnect from Server") == 0) {
						cout << "Client disconnected from Server\n";
						break;
					}
				} while (1);
			}
			connector.Close();
		}
		server_socket.Close();
	}

	return n_ret_code;
}
