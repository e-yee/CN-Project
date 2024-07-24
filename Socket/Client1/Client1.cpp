#include "stdafx.h"
#include "Client1.h"
#include <afxsock.h>
#include <cstdlib>
#include <csignal>
#include <conio.h>
#include <fstream>
#include <vector>
#include <queue>
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinApp theApp;

using namespace std;

CSocket ClientSocket;

void signalHandler(int signum)
{
	const char* termination_message = "1111 - Disconnect from Server";
	int message_size = static_cast<int>(strlen(termination_message));

	if (ClientSocket.m_hSocket != INVALID_SOCKET) {
		ClientSocket.Send(&message_size, sizeof(message_size), 0);
		ClientSocket.Send(termination_message, message_size, 0);
		ClientSocket.Close();
	}
	exit(signum);
}

void registerAvailibleFile()
{
	ofstream output("request.txt");
	if (!output.good()) return;

	int message_size = 0;
	char* message;

	ClientSocket.Receive((char*)(&message_size), sizeof(int), 0);
	message = new char[message_size + 1];
	ClientSocket.Receive(message, message_size, 0);

	message[message_size] = '\0';
	output << message;

	cout << message << "\n";
	output.close();
}

void registerRequestingFiles(queue<string>& requesting_files)
{
	string fuck = "input.txt";
	ifstream file(fuck);

	if (!file.good())
	{
		cout << "Unable to read input!\n";
		exit(1);
	}

	string file_name;
	queue<string> temp = requesting_files;

	while (getline(file, file_name))
	{
		requesting_files.push(file_name);
	}

	file.close();
	ofstream file1(fuck);
	file1.close();
}

bool receiveDownloadData(string file_name)
{
	////make a thread for this
	//string path = "D:\\output\\" + file_name;
	//ofstream file(path, ios::binary);
	//ofstream debugging("debug.txt", ios::app);

	//char buffer[10240];
	//int buffer_size = 10240;

	//int file_size = 0;
	//float current_progress = 0;
	//int bytes_received = 0;
	//int a = 0; //debug

	//ClientSocket.Receive((char*)(&file_size), sizeof(int), 0);
	//debugging << file_size << "\n";
	//int total_progress = file_size;

	//int i = 0;
	//while (file_size >= 10240)
	//{
	//	a = ClientSocket.Receive((char*)(&buffer_size), sizeof(int), 0);
	//	bytes_received = ClientSocket.Receive(buffer, buffer_size, 0);

	//	ClientSocket.Send(&bytes_received, sizeof(bytes_received), 0);

	//	if (bytes_received != 10240)
	//	{
	//		cout << "wrong " << i << "\n";
	//		_flushall();
	//		continue;
	//	}

	//	cout << a << " | " << bytes_received << " | " << buffer_size << " | " << (current_progress / total_progress) * 100 << "%\n";
	//	debugging << a << " | " << bytes_received << " | " << buffer_size << " | " << (current_progress / total_progress) * 100 << "%\n";

	//	file_size -= 10240;
	//	file.write(buffer, buffer_size);
	//	current_progress += buffer_size;
	//	i++;
	//}
	//
	//if (file_size > 0)
	//{
	//	ClientSocket.Receive((char*)(&buffer_size), sizeof(int), 0);
	//	char* ptr_buffer = new char[file_size];
	//	bytes_received = ClientSocket.Receive(ptr_buffer, file_size, 0);

	//	file.write(ptr_buffer, buffer_size);
	//	delete[] ptr_buffer;

	//	cout << a << " | " << bytes_received << " | " << buffer_size << " | " << (current_progress / total_progress) * 100 << "%\n";
	//	debugging << a << " | " << bytes_received << " | " << buffer_size << " | " << (current_progress / total_progress) * 100 << "%\n";
	//}

	//debugging.close();
	//file.close();

	string path = "D:\\output\\" + file_name;
	ofstream ofs(path.c_str(), ios::binary);

	int file_size = 0;
	ClientSocket.Receive((char*)&file_size, sizeof(int), 0);

	int bytes_received = 0;
	int buffer_size = 0;
	char buffer[10240] = {};

	while (file_size >= 10240) {
		ClientSocket.Receive((char*)&buffer_size, sizeof(int), 0);
		bytes_received = ClientSocket.Receive(buffer, buffer_size, 0);

		ClientSocket.Send(&bytes_received, sizeof(int), 0);

		//Server resends until Client can receive data chunk
		while (bytes_received == -1) {
			ClientSocket.Receive((char*)&buffer_size, sizeof(int), 0);
			bytes_received = ClientSocket.Receive(buffer, buffer_size, 0);

			ClientSocket.Send(&bytes_received, sizeof(int), 0);
		}

		//Receive the rest of the data chunk
		while (bytes_received < buffer_size) {
			ofs.write(buffer, bytes_received);

			ClientSocket.Receive((char*)&buffer_size, sizeof(int), 0);
			bytes_received = ClientSocket.Receive(buffer, buffer_size, 0);

			ClientSocket.Send(&bytes_received, sizeof(int), 0);

			//Server resends until Client can receive data chunk
			while (bytes_received == -1) {
				ClientSocket.Receive((char*)&buffer_size, sizeof(int), 0);
				bytes_received = ClientSocket.Receive(buffer, buffer_size, 0);

				ClientSocket.Send(&bytes_received, sizeof(int), 0);
			}
		}

		ofs.write(buffer, bytes_received);
		file_size -= 10240;
	}

	bytes_received = 0;
	char* rest_buffer;
	while (file_size > 0) {
		rest_buffer = new char[file_size];

		ClientSocket.Receive((char*)&buffer_size, sizeof(int), 0);
		bytes_received = ClientSocket.Receive(rest_buffer, buffer_size, 0);

		ClientSocket.Send(&bytes_received, sizeof(int), 0);

		//Server resends until Client can receive data chunk
		while (bytes_received == -1) {
			ClientSocket.Receive((char*)&buffer_size, sizeof(int), 0);
			bytes_received = ClientSocket.Receive(rest_buffer, buffer_size, 0);

			ClientSocket.Send(&bytes_received, sizeof(int), 0);
		}

		ofs.write(rest_buffer, bytes_received);
		file_size -= bytes_received;

		delete[] rest_buffer;
	}

	ofs.close();
	return true;
}

bool processRequestingFile(queue<string>& requesting_files)
{
	if (requesting_files.empty()) return false;

	int file_size = requesting_files.front().size();

	ClientSocket.Send(&file_size, sizeof(file_size), 0);
	ClientSocket.Send(requesting_files.front().c_str(), file_size, 0);
	return true;
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[]) {
	signal(SIGINT, signalHandler);

	int nRetCode = 0;

	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		nRetCode = 1;
		return nRetCode;
	}
	if (AfxSocketInit() == FALSE) {
		cout << "Socket Library initialization failed\n";
		return FALSE;
	}

	ClientSocket.Create();
	if (ClientSocket.Connect(_T("127.0.0.1"), 1234) == 0)
	{
		cout << "Server connection fucked\n";
		return nRetCode;
	}
	cout << "Server connection succeeded\n";

	registerAvailibleFile();

	queue<string> requesting_files;

	while (1)
	{
		registerRequestingFiles(requesting_files);
		processRequestingFile(requesting_files);
		if (!requesting_files.empty())
		{
			cout << "downloading...\n";
			if (receiveDownloadData(requesting_files.front()))
			{
				//file done
				cout << "file done!\n";
				signalHandler(1);
			}
			requesting_files.pop();
		}
		Sleep(100);
	}
	ClientSocket.Close();

	return nRetCode;
}