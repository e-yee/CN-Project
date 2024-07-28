#include "stdafx.h"
#include "Client1.h"
#include <afxsock.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinApp theApp;

using namespace std;

CSocket ClientSocket;

void signalHandler(int signum)
{
	if (ClientSocket.m_hSocket != INVALID_SOCKET) {
		int termination_code = -3;
		ClientSocket.Send(&termination_code, sizeof(int), 0);
		ClientSocket.Close();
	}

	ofstream file("input.txt");
	file.close();

	exit(signum);
}

void displayDownloadProgress(float progress, float total, string file_name)
{
	float percentage = progress / total;

	cout << "\rDownloading: " << "\"" << file_name << "\"" << ": [";

	if (percentage >= 1) {
		for (int i = 0; i < 10; i++) {
			cout << "\xDB\xDB";
		}
		cout << "] 100%";
		return;
	}

	int progress_bar = percentage * 10;
	for (int i = 0; i < progress_bar; i++) {
		cout << "\xDB\xDB";
	}
	for (int i = progress_bar; i < 10; i++) {
		cout << "  ";
	}
	cout << "] " << percentage * 100 << setprecision(3) << "%          ";
}

void registerAvailibleFile()
{
	int message_size = 0;
	char* message;

	ClientSocket.Receive((char*)(&message_size), sizeof(int), 0);
	message = new char[message_size + 1];
	ClientSocket.Receive(message, message_size, 0);

	message[message_size] = '\0';
	cout << "           <<< Available files >>>\n";

	cout << message << "\n";
}

void registerRequestingFiles(queue<string>& requesting_files)
{
	string fuck = "input.txt";
	ifstream file(fuck);

	if (!file.good())
	{
		cout << "           <<< Unable to open Input File! >>>\n";
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
		while (bytes_received == -1) {
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
			while (bytes_received == -1) {
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
		while (bytes_received == -1) {
			bytes_received = ClientSocket.Receive(rest_buffer, file_size, 0);

			ClientSocket.Send(&bytes_received, sizeof(int), 0);
		}

		ofs.write(rest_buffer, bytes_received);
		file_size -= bytes_received;

		delete[] rest_buffer;
	}

	displayDownloadProgress(part, total, file_name);
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
		_tprintf(_T("<<< Fatal Error: MFC initialization failed >>>\n"));
		nRetCode = 1;
		return nRetCode;
	}
	if (AfxSocketInit() == FALSE) {
		cout << "<<< Fatal Error: Socket Library initialization failed >>>\n";
		return FALSE;
	}

	ClientSocket.Create();
	int try_time = 0;
	while (ClientSocket.Connect(_T("192.168.1.11"), 1234) == 0)
	{
		cout << "\r<<< Server connection failed... Trying again...>>>\n";
		Sleep(500);
		try_time++;
		if (try_time >= 3) {
			cout << "<<< Server connection failed >>>\n";
			return nRetCode;
		}
	}
	cout << "<<< Server connection succeeded >>>\n\n\n";

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
			}
			requesting_files.pop();

			ClientSocket.Receive(&response, sizeof(int), 0);
		}

		if (response == 0) {
			cout << "           <<< All available files downloaded. Disconnecting... >>>\n\n\n";
			break;
		}
	}

	ClientSocket.Close();
	cout << "           <<< Disconnected! >>>\n";

	return nRetCode;
}