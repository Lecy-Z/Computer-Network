#undef UNICODE
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <iostream>
#include <streambuf> 
#include <fstream>
#include <string>
#include <thread>
#include <map>
#include <vector>
#include <mutex>
#include <ctime>
#include <ratio>
#include <chrono>

using namespace std;
// socket�����Ҫ���õĿ�
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define SERVER_NAME "2473-13107FF"
#define DEFAULT_PORT 4579
#define DEFAULT_BUFLEN 512

struct Client {
	char* IP;
	int port;
	int id;
	thread::id thread_id;
	SOCKET socket;
};
vector<struct Client*> client_list;
int client_num=0;
std::mutex thread_lock;

//��stringת��Ϊchar*���ͺ󣬵���socket��send����
void SendString(SOCKET s, string str);
//����ͻ��˷�����ʱ������
void GetTime(SOCKET s);
//����ͻ��˷�������������
void GetServerName(SOCKET s);
//����ͻ��˷������б�����
void GetClientList(SOCKET s);
//�ж�һ���ͻ����Ƿ�������б���
bool isAlive(int id);
//��Ѱ�б��пͻ��˶�Ӧ���׽���
SOCKET SearchSocket(int id);
//�����ͻ��˷������������ݰ�
int ProcessRequestPacket(SOCKET s, char* rec_packet);
//ÿ���ͻ��˶�Ӧ�����̴߳�����
void ClientThread(SOCKET sClient);

int main() {
	
	WORD wVersionRequested;
	WSADATA wsaData;
	int ret, nLeft, length;
	SOCKET sListen, sServer; //�����׽��֣������׽���
	struct sockaddr_in saServer;//��ַ��Ϣ
	SOCKET sClient;
	char ch;

	printf("Welcome! Here is a server created by 3170102473 & 3170104579\n");
	printf("Server initilizing...\n");

	//WinSock��ʼ����
	wVersionRequested = MAKEWORD(2, 2);//ϣ��ʹ�õ�WinSock DLL�İ汾
	ret = WSAStartup(wVersionRequested, &wsaData);
	if (ret != 0) {
		printf("WSAStartup() failed!\n");
		ch=getchar();
		return 0;
	}
	//ȷ��WinSock DLL֧�ְ汾2.2��
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)	{
		WSACleanup();
		printf("Invalid Winsock version!\n");
		ch = getchar();
		return 0;
	}
	//����socket��ʹ��TCPЭ�飺
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sListen == INVALID_SOCKET)
	{
		WSACleanup();
		printf("socket() failed!\n");
		ch = getchar();
		return 0;
	}
	//�������ص�ַ��Ϣ��
	saServer.sin_family = AF_INET;//��ַ����
	saServer.sin_port = htons(DEFAULT_PORT);//ע��ת��Ϊ�����ֽ���
	saServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//ʹ��INADDR_ANYָʾ�����ַ
	//�󶨣�
	ret = bind(sListen, (struct sockaddr*) & saServer, sizeof(saServer));
	if (ret == SOCKET_ERROR){
		printf("bind() failed! code:%d\n", WSAGetLastError());
		closesocket(sListen);//�ر��׽���
		WSACleanup();
		ch = getchar();
		return 0;
	}
	//������������,�������ӵȴ����鳤��Ϊ5
	ret = listen(sListen, 5);
	if (ret == SOCKET_ERROR){
		printf("listen() failed! code:%d\n", WSAGetLastError());
		closesocket(sListen);//�ر��׽���
		WSACleanup();
		ch = getchar();
		return 0;
	}
	printf("Success! Waiting for clients...\n");
    //ѭ������accept���ȴ��ͻ���
	for (;;) {
		sClient = accept(sListen, NULL, NULL);
		if (sClient == INVALID_SOCKET) { // accept����
			printf("accept() failed! code:%d\n", WSAGetLastError());
			closesocket(sClient);
			WSACleanup();
			ch = getchar();
			return 0;
		}
		//����һ���µ����̣߳��������õķ�ʽ����socket
		thread(ClientThread, ref(sClient)).detach();
	}
	//�ر�socket
	closesocket(sListen);
	WSACleanup();
	return 0;
}

// ����string���͵����ݰ���ͨ��SOCKET s���ͳ�ȥ
void SendString(SOCKET s, string str) {
	char content[DEFAULT_BUFLEN];
	strcpy(content, str.c_str());
	int len = str.length();
	send(s, content, len, 0);
}

// �ͻ������̴߳�����
void ClientThread(SOCKET sClient) {
	char rec_packet[DEFAULT_BUFLEN]="\0"; 
	char* IP = NULL;
	int port;

	SOCKADDR_IN client_info = { 0 };
	int addrsize = sizeof(client_info);
	// ��ȡ�̱߳��
	thread::id thread_id = this_thread::get_id();
	// ��ȡ�ͻ��˵�IP��˿�
	getpeername(sClient, (struct sockaddr*) & client_info, &addrsize);
	IP = inet_ntoa(client_info.sin_addr);
	port = client_info.sin_port;
	printf("Connected to client: %s\n", IP);
	//����һ���µ�Client����
	struct Client c;
	c.socket = sClient; c.IP = IP; c.id = client_num++;
	c.port = port; c.thread_id = thread_id;
	struct Client* this_client = &c;
	//�����Client�������е��б���д������ǰ����
	thread_lock.lock();
	client_list.push_back(this_client);
	thread_lock.unlock(); //д�����ݺ����
	
	//ѭ������recv()�ȴ��ͻ��˵��������ݰ�
	int ret = 0;
	bool is_close = false;
	while (!is_close) {
		ZeroMemory(rec_packet, DEFAULT_BUFLEN);
		ret = recv(sClient, rec_packet, DEFAULT_BUFLEN, 0);
		if (ret > 0) { //����
			printf("Packet received! Processing...\n");
			//����T,N,L�������󣬼�����ʱ�䡢�������������ͻ����б�
			int flag = ProcessRequestPacket(sClient, rec_packet);
			if (flag==0) { //����M�������󣬼�������Ϣ
				// ��ָ���ͻ��˷�����Ϣ
				string str = rec_packet;  
				int pos1, pos2;
				pos1 = str.find("*") + 1;
				pos2 = str.find("*", pos1);
				int to_id = stoi(str.substr(pos1, pos2 - pos1)); // ��ȡĿ�ĵ�ID
				pos1 = pos2 + 1;
				pos2 = str.find("#", pos1);
				string message = str.substr(pos1, pos2 - pos1); // ��ȡ��Ϣ����
				if (isAlive(to_id)) {
					SOCKET sDestination = SearchSocket(to_id); // ��ȡĿ��ͻ��˵�socket
					//��װָʾ���ݰ���ת����Ϣ
					string response = "#M-M*";
					response += to_string(this_client->id);
					response += " ";
					response += this_client->IP;
					response += "\n";
					response += message;
					response += "#\0";
					SendString(sDestination, response);
					printf("Send a message to destination client!\n");
					//������Ӧ���ݰ�����ʾ���ͳɹ�
					response = "#M-Y*";
					response += to_string(to_id);
					response += " ";
					response += message;
					response += "#\0";
					SendString(sClient, response);
					printf("Send a message to start client!\n");
				}
				else {
					// ������Ӧ���ݰ�����ʾ����ʧ��
					string response = "#M-N*Destination doesn't exist!#";
					SendString(sClient, response);
					printf("Send message failed!\n");
				}
			}
		}
		else if (ret == 0) { //�����ж�
			printf("Connection closed!\n");
			is_close = true;
		}
		else { //���г���
			printf("recv() failed! code:%d\n", WSAGetLastError());
			closesocket(sClient);
			WSACleanup();
			return;
		}
	}
	//ֹͣ��ÿͻ��˵�����
	closesocket(sClient);
	vector<struct Client*>::iterator it;
	for (it = client_list.begin(); it != client_list.end(); it++) {
		if ((*it)->id == this_client->id && (*it)->IP == this_client->IP) {
			break;
		}
	}
	//���б���ɾȥ�ÿͻ���
	thread_lock.lock();
	client_list.erase(it);
	client_num--;
	//delete this_client;
	thread_lock.unlock();
	return;
}

int ProcessRequestPacket(SOCKET s, char* rec_packet){
	if (rec_packet[0] != '#') {
		printf("It's not a packet!\n");
		return -1;
	}
	char packet_type = rec_packet[1];
	switch (packet_type) {
	case 'T':
		GetTime(s);
		break;
	case 'N':
		GetServerName(s);
		break;
	case 'L':
		GetClientList(s);
		break;
	default:
		return 0;
		break;
	}
	return 1;
}

void GetTime(SOCKET s) {
	//for (int i = 0; i < 100; i++) {
		string response = "#T*";
		using std::chrono::system_clock;
		system_clock::time_point today = system_clock::now();
		std::time_t tt;
		tt = system_clock::to_time_t(today);
		response += ctime(&tt);
		response += "#\0";
		SendString(s, response);
	//	Sleep(500);
//	}	
	printf("Time send.\n");
}

void GetServerName(SOCKET s) {
	string response = "#N*";
	response += SERVER_NAME;
	response += "#\0";
	SendString(s, response);
	printf("Server name send.\n");
}

void GetClientList(SOCKET s) {
	string response = "#L*";
	thread_lock.lock();
	vector<struct Client*>::iterator it;
	for (it = client_list.begin(); it != client_list.end(); it++) {
		response += to_string((*it)->id);
		response += ' ';
		response += (*it)->IP;
		response += '\n';
	}
	response += "#\0";
	thread_lock.unlock();
	SendString(s, response);
	printf("Client list send.\n");
}

bool isAlive(int id) {
	bool flag = false;
	thread_lock.lock();
	vector<struct Client*>::iterator it;
	for (it = client_list.begin(); it != client_list.end(); it++) {
		if ((*it)->id == id) {
			flag = true;
			break;
		}
	}
	thread_lock.unlock();
	return flag;
}

SOCKET SearchSocket(int id) {
	SOCKET socket;
	thread_lock.lock();
	vector<struct Client*>::iterator it;
	for (it = client_list.begin(); it != client_list.end(); it++) {
		if ((*it)->id == id) {
			socket = (*it)->socket;
			break;
		}
	}
	thread_lock.unlock();
	return socket;
}