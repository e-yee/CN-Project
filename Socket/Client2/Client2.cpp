// Demo_Client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Client2.h"
#include "afxsock.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinApp theApp;

using namespace std;
using std::cout;


struct File
{
	string name;
	string priority;
};

struct DataChunk
{
	string file_name;
	string position;
	string chunk_size;
	string content;
};

CSocket ClientSocket;
queue<File> requesting_files;

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

void displayDownloadProgress(float progress, float total, string file_name, int order)
{
	float percentage = progress / total;

	cout << "\x1b[" << 0 << ";" << order + 10 << "H";
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

void registerRequestingFiles(queue<File>& requesting_files)
{
	string fuck = "input.txt";
	ifstream file(fuck);

	if (!file.good())
	{
		cout << "           <<< Unable to open Input File! >>>\n";
		exit(1);
	}

	File temp;

	do {
		file >> temp.name >> temp.priority;
		requesting_files.push(temp);
	} while (!file.eof());

	file.close();
}



bool receiveDownloadData(string file_name)
{
	string path = "C:\\Users\\PC\\Desktop\\" + file_name;
	ofstream ofs(path.c_str(), ios::binary);

	/*int max_chunk_size = 10240;//

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

	displayDownloadProgress(1, 1, file_name);
	ofs.close();
	return true; */
}

void sendRequestingFile(queue<File> requesting_files)
{
	if (requesting_files.empty()) return;

	string back = requesting_files.back().name;
	string message;
	int message_size;

	while (!requesting_files.empty()) {
		message = requesting_files.front().name + " " + requesting_files.front().priority;
		if (back != requesting_files.front().name) message += "\n";
	}
	message_size = message.size();

	ClientSocket.Send(&message_size, sizeof(int));
	ClientSocket.Send(message.c_str(), message_size);
	requesting_files.pop();
}

DWORD WINAPI checkInputFile(LPVOID arg)
{
	SOCKET* cock = (SOCKET*)arg;
	CSocket pp;
	pp.Attach(*cock);

	while (1) {
		queue<File> temp = requesting_files;
		registerRequestingFiles(requesting_files);

		if (temp.back().name != requesting_files.back().name) {
			sendRequestingFile(requesting_files);
		}
		Sleep(2000);
	}
}

DWORD WINAPI downloadFiles(LPVOID arg)
{
	SOCKET* cock = (SOCKET*)arg;
	CSocket pp;
	pp.Attach(*cock);

	//aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
	//aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
	//aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
	//aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
	//aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
	//aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
	//aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
	//aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
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
	while (ClientSocket.Connect(_T("192.168.1.11"), 1234) != 0)
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

	DWORD threadID;
	HANDLE thread_status;
	SOCKET* balls = new SOCKET();
	*balls = ClientSocket.Detach();

	registerAvailibleFile();
	thread_status = CreateThread(0, 0, checkInputFile, balls, 0, &threadID);

	vector<string> downloaded;
	string file_download;
	int response = -1;

	while (1) {
		//
		while (1) {
			if (receiveDownloadData(requesting_files.front().name)) {
				downloaded.push_back(requesting_files.front().name);
			}
			requesting_files.pop();

			ClientSocket.Receive(&response, sizeof(int), 0);
			break;
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

/*
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
			client.Connect(_T("192.168.1.11"), 1234);

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
*/
