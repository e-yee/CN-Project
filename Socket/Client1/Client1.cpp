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
#include <iomanip>

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

	ofstream file("input.txt");
	file.close();

	exit(signum);
}

void displayDownloadProgress(int progress, int total, string file_name)
{
	float percentage = progress / (1.0 * total);

	cout << "\rDownloading: " << "\"" << file_name << "\"" << ": [";
	cout << "                              ";
	cout << percentage * 100 << setprecision(3) << "%";
	for (float i = 0; i < percentage; i += 0.1) {
		cout << "\xDB\xDB";
	}

	if (percentage == 1) cout << "]";
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
}

bool receiveDownloadData(string file_name)
{
	string path = "C:\\Users\\PC\\Desktop\\" + file_name;
	ofstream ofs(path.c_str(), ios::binary);

	int max_chunk_size = 10240;//

	int file_size = 0;
	ClientSocket.Receive((char*)&file_size, sizeof(int), 0);
	int total = file_size;
	int part = 0;
	
	int chunk = 10240;
	int bytes_received = 0;
	int buffer_size = 0;
	char buffer[10240] = {};

	while (file_size >= 10240) {
		chunk = 10240;
		bytes_received = ClientSocket.Receive(buffer, chunk, 0);

		ClientSocket.Send(&bytes_received, sizeof(int), 0);

		//Server resends until Client can receive data chunk
		while (bytes_received == -1 || bytes_received == 0) {
			bytes_received = ClientSocket.Receive(buffer, chunk, 0);
			ClientSocket.Send(&bytes_received, sizeof(int), 0);
		}

		//Receive the rest of the data chunk
		while (bytes_received < chunk) {
			ofs.write(buffer, bytes_received);
			chunk -= bytes_received;

			bytes_received = ClientSocket.Receive(buffer, chunk, 0);

			ClientSocket.Send(&bytes_received, sizeof(int), 0);

			//Server resends until Client can receive data chunk
			while (bytes_received == -1 || bytes_received == 0) {
				bytes_received = ClientSocket.Receive(buffer, chunk, 0);
				ClientSocket.Send(&bytes_received, sizeof(int), 0);
			}

			if (chunk == bytes_received) break;
		}

		ofs.write(buffer, bytes_received);
		file_size -= 10240;
		part += 10240;
		displayDownloadProgress(part, total, file_name);
	}

	bytes_received = 0;
	char* rest_buffer;
	while (file_size > 0) {
		rest_buffer = new char[file_size];

		bytes_received = ClientSocket.Receive(rest_buffer, file_size, 0);

		ClientSocket.Send(&bytes_received, sizeof(int), 0);

		//Server resends until Client can receive data chunk
		while (bytes_received == -1 || bytes_received == 0) {
			bytes_received = ClientSocket.Receive(rest_buffer, file_size, 0);

			ClientSocket.Send(&bytes_received, sizeof(int), 0);
		}

		ofs.write(rest_buffer, bytes_received);
		file_size -= bytes_received;

		delete[] rest_buffer;
	}

	ofs.close();
	return true;
}

bool processRequestingFile(queue<string> requesting_files)
{
	if (requesting_files.empty()) return false;
	
	string message = requesting_files.front();
	int message_size = message.size();

	ClientSocket.Send(&message_size, sizeof(int), 0);
	ClientSocket.Send(message.c_str(), message_size, 0);

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
	int try_time = 0;
	while (ClientSocket.Connect(_T("192.168.1.11"), 1234) == 0)
	{
		cout << "Server connection fucked\n";
		cout << "Press Any key to try again\n";
		char ass = _getch();
		Sleep(500);
		try_time++;
		if (try_time >= 3) {
			cout << "Server connection absolutely fucked\n";
			return nRetCode;
		}
	}
	cout << "Server connection succeeded\n";

	registerAvailibleFile();

	queue<string> requesting_files;
	vector<string> downloaded;
	string file_download;
	int response = -1;

	while (1) {
		registerRequestingFiles(requesting_files);

		if (!requesting_files.empty()) {
			while (find(downloaded.begin(), downloaded.end(), requesting_files.front()) != downloaded.end()) {
				requesting_files.pop();

				if (requesting_files.empty()) break;
			}
		}

		while (!requesting_files.empty()) {
			processRequestingFile(requesting_files);

			if (receiveDownloadData(requesting_files.front())) {
				downloaded.push_back(requesting_files.front());
				cout << "Downloading " << requesting_files.front() << " 100%\n";
			}
			requesting_files.pop();

			ClientSocket.Receive(&response, sizeof(int), 0);
		}

		if (response == 0) {
			cout << "Client has download all files so fuck off\n";
			break;
		}
	}

	ClientSocket.Close();

	return nRetCode;
}