#include "stdafx.h"
#include "Server1.h"
#include <afxsock.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinApp theApp;

using namespace std;

CSocket server_socket;

struct File {
	string name, size;
};

void signalHandler(int signum)
{
	const char* termination_message = "404 - Server disconnection";
	int message_size = static_cast<int>(strlen(termination_message));

	if (server_socket.m_hSocket != INVALID_SOCKET) {
		server_socket.Send(&message_size, sizeof(message_size), 0);
		server_socket.Send(termination_message, message_size, 0);
		server_socket.Close();
	}
	exit(signum);
}

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

void sendFileList(vector<File> &file_list, CSocket& connector) {
	string file_name = "Resource Files\\download.txt";
	file_list = getFileList(file_name);

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

	int bytes_sent = 0;
	int bytes_received = 0;
	int bytes_offset = 0;
	int chunk_size = 10240;
	char chunk[10240] = {};

	while (real_size >= 10240) {
		ifs.read(chunk, chunk_size);

		bytes_sent = chunk_size;
		connector.Send(&bytes_sent, sizeof(int), 0);
		connector.Send(chunk, bytes_sent, 0);

		connector.Receive((char*)&bytes_received, sizeof(int), 0);

		//Resend if Client cannot receive data chunk
		while (bytes_received == -1 || bytes_received == 0) {
			connector.Send(&bytes_sent, sizeof(int), 0);
			connector.Send(chunk, bytes_sent, 0);

			connector.Receive((char*)&bytes_received, sizeof(int), 0);
		}

		//Send the rest until Client receives full data chunk
		while (bytes_received < bytes_sent) {
			bytes_sent -= bytes_received;
			ifs.seekg(-bytes_sent, ios::cur);
			ifs.read(chunk, bytes_sent);

			connector.Send(&bytes_sent, sizeof(int), 0);
			connector.Send(chunk, bytes_sent, 0);

			connector.Receive((char*)&bytes_received, sizeof(int), 0);

			//Resend if Client cannot receive data chunk
			while (bytes_received == -1 || bytes_received == 0) {
				connector.Send(&bytes_sent, sizeof(int), 0);
				connector.Send(chunk, bytes_sent, 0);

				connector.Receive((char*)&bytes_received, sizeof(int), 0);
			}
		}
		real_size -= chunk_size;
	}

	bytes_sent = bytes_received = 0;
	char* rest_chunk;
	while (real_size > 0) {
		rest_chunk = new char[real_size];

		bytes_offset = bytes_sent - bytes_received;
		ifs.seekg(-bytes_offset, ios::cur);
		ifs.read(rest_chunk, real_size);

		bytes_sent = real_size;
		connector.Send(&bytes_sent, sizeof(int), 0);
		connector.Send(rest_chunk, bytes_sent, 0);

		connector.Receive((char*)&bytes_received, sizeof(int), 0);

		//Resend if Client cannot receive data chunk
		while (bytes_received == -1 || bytes_received == 0) {
			connector.Send(&bytes_sent, sizeof(int), 0);
			connector.Send(rest_chunk, bytes_sent, 0);

			connector.Receive((char*)&bytes_received, sizeof(int), 0);
		}

		real_size -= bytes_received;
		delete[] rest_chunk;
	}
	cout << "Uploading completed!\n";

	ifs.close();
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
				
				//Uploading process
				int response = -1;
				int message_size = 0;
				char* message;
				string requesting_file;
				
				do {
					connector.Receive((char*)&message_size, sizeof(int), 0);
					message = new char[message_size + 1];
					connector.Receive(message, message_size, 0);
					message[message_size] = '\0';

					if (strcmp(message, "1111 - Disconnect from Server") == 0) {
						cout << "Client disconnected from Server!\n";
						break;
					}

					requesting_file = message;
					response = -1;

					for (int i = 0; i < file_list.size(); ++i)
						if (file_list[i].name == requesting_file) {
							response = 1;

							uploadProcess(requesting_file, connector);
							file_list.erase(file_list.begin() + i);
							break;
						}
						
					if (file_list.empty()) {
						response = 0;

						cout << "Client has downloaded all files\n";
						connector.Send((char*)&response, sizeof(int), 0);

						break;
					}
					connector.Send((char*)&response, sizeof(int), 0);
				} while (1);
			}
			connector.Close();
		}
		server_socket.Close();
	}

	return n_ret_code;
}
