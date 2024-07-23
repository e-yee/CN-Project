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
	int real_size = ifs.tellg();
	ifs.seekg(0, ios::beg);

	connector.Send(&real_size, sizeof(int), 0);

	int byte_sent = 0;
	int byte_received = 0;
	int byte_offset = 0;
	int chunk_size = 10240;
	char chunk[10240] = {};

	while (real_size >= 10240) {
		ifs.read(chunk, chunk_size);

		byte_sent = chunk_size;
		connector.Send(&byte_sent, sizeof(int), 0);
		connector.Send(chunk, byte_sent, 0);

		connector.Receive((char*)&byte_received, sizeof(int), 0);

		while (byte_received == -1) {
			connector.Send(&byte_sent, sizeof(int), 0);
			connector.Send(chunk, byte_sent, 0);

			connector.Receive((char*)&byte_received, sizeof(int), 0);
		}

		while (byte_received < byte_sent) {
			byte_sent -= byte_received;
			ifs.seekg(-byte_sent, ios::cur);
			ifs.read(chunk, byte_sent);

			connector.Send(&byte_sent, sizeof(int), 0);
			connector.Send(chunk, byte_sent, 0);

			connector.Receive((char*)&byte_received, sizeof(int), 0);

			while (byte_received == -1) {
				connector.Send(&byte_sent, sizeof(int), 0);
				connector.Send(chunk, byte_sent, 0);

				connector.Receive((char*)&byte_received, sizeof(int), 0);
			}
		}
		real_size -= 10240;
	}

	if (real_size > 0) {
		byte_sent = byte_received = 0;
		char* rest_chunk;
		do {
			real_size -= byte_received;
			rest_chunk = new char[real_size];

			byte_offset = byte_sent - byte_received;
			ifs.seekg(-byte_offset, ios::cur);
			ifs.read(rest_chunk, real_size);

			byte_sent = real_size;
			connector.Send(&byte_sent, sizeof(int), 0);
			connector.Send(rest_chunk, byte_sent, 0);

			connector.Receive((char*)&byte_received, sizeof(int), 0);

			while (byte_received == -1) {
				connector.Send(&byte_sent, sizeof(int), 0);
				connector.Send(rest_chunk, byte_sent, 0);

				connector.Receive((char*)&byte_received, sizeof(int), 0);
			}

			delete[] rest_chunk;

			if (byte_received == byte_sent) break;
		} while (1);
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
