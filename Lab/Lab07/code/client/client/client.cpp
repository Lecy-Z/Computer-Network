#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
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
#include <map>
#include <mutex>
#include <ctime>
#include <ratio>
#include <chrono>
using namespace std;
//socket�����Ҫ���õĿ�
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "4579"

mutex mtx_time;
mutex mtx_name;
mutex mtx_list;
mutex mtx_message;
mutex mtx_output;
condition_variable Time;
condition_variable name;
condition_variable list;
condition_variable message;

//�˵��б�
void helpinfo();
//�ͻ��˽�����Ϣ�������Ϣ
void output(string response_content);
//�ͻ��˽�����Ϣ
int receivePacket(SOCKET connect_socket);
//�ͻ������������˶Ͽ�����
void closeConnect(SOCKET connect_socket);
//�ͻ���������ʱ��
void getTime(char* send_buf, SOCKET connect_socket);
//�ͻ��������÷���������
void getName(char* send_buf, SOCKET connect_socket);
//�ͻ��������ÿͻ����б�
void getList(char* send_buf, SOCKET connect_socket);
//�ͻ�����������Ϣ
void sendMessage(char* send_buf, SOCKET connect_socket);
//�ͻ��������˳�
void dropout(SOCKET connect_socket);

int main()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	SOCKET connect_socket = INVALID_SOCKET;
	struct addrinfo* result = NULL;
	struct addrinfo* ptr = NULL;
	struct addrinfo hints;
	char send_buf[DEFAULT_BUFLEN]; //Ҫ���͵���Ϣ
	int ret;
	char ch;
	int recv_buflen = DEFAULT_BUFLEN; //�յ�����Ϣ
	char ip[DEFAULT_BUFLEN];
	ZeroMemory(send_buf, DEFAULT_BUFLEN);
	while (1)
	{

		//��ʼ����
		helpinfo();

		int c;
		cin >> c;
		if (c == 2 || c == 7)
		{ //�˳�
			return 0;
		}
		else if (c == 1)
		{ //���ӷ�����
			printf("please input the server ip:\n");
			cin >> ip;
			if (ip)
				cout << "usage: " << ip << " server-name" << endl;

			//Winsock��ʼ��
			wVersionRequested = MAKEWORD(2, 2); //ϣ��ʹ�õ�WinSock DLL�İ汾
			ret = WSAStartup(wVersionRequested, &wsaData);
			if (ret != 0)
			{
				cout << "WSAStartup failed with error: " << to_string(ret) << endl;
				system("pause");
				continue;
			}

			//ȷ��WinSock DLL֧�ְ汾2.2��
			if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
			{
				WSACleanup();
				printf("Invalid Winsock version!\n");
				ch = getchar();
				return 0;
			}

			//�������ص�ַ��Ϣ
			ZeroMemory(&hints, sizeof(hints));
			hints.ai_family = AF_UNSPEC;     //��ַ����
			hints.ai_socktype = SOCK_STREAM; //socket����
			hints.ai_protocol = IPPROTO_TCP; //protocol

			//�����������˿ں͵�ַ
			ret = getaddrinfo(ip, DEFAULT_PORT, &hints, &result);
			if (ret != 0)
			{
				cout << "getaddrinfo failed with error: " << to_string(ret) << endl;
				WSACleanup();
				continue;
			}
			int flag = 0;
			for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
			{
				//�����������ӷ�������SOCKET��ʹ��TCPЭ��
				connect_socket = socket(ptr->ai_family, ptr->ai_socktype,
					ptr->ai_protocol);
				if (connect_socket == INVALID_SOCKET)
				{
					cout << "socket failed with error: " << to_string(WSAGetLastError()) << endl;
					WSACleanup();
					flag = 1;
					break;
				}

				//���ӵ�������
				ret = connect(connect_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
				if (ret == SOCKET_ERROR)
				{
					closesocket(connect_socket);
					connect_socket = INVALID_SOCKET;
					continue;
				}
				break;
			}
			if (flag == 1)
			{
				continue;
			}
			else
			{
				freeaddrinfo(result);
				if (connect_socket == INVALID_SOCKET)
				{
					cout << "Unable to connect to server!" << endl;
					WSACleanup();
					continue;
				}
				else
				{ //���ӳɹ�
					cout << "Connection succeed!" << endl;
					char* IP = NULL;
					int port = 0;

					//��ÿͻ�����Ϣ
					SOCKADDR_IN clientInfo = { 0 };
					int addr_size = sizeof(clientInfo);
					thread::id this_id = this_thread::get_id();

					//������ڵ�ip��port
					getpeername(connect_socket, (struct sockaddr*) & clientInfo, &addr_size);
					IP = inet_ntoa(clientInfo.sin_addr);
					port = clientInfo.sin_port;
					cout << "current IP: " << IP << endl;

					thread(receivePacket, move(connect_socket)).detach(); //��������

					while (1)
					{
						helpinfo();
						int a;
						cin >> a;
						if (a == 2)
						{ //�����������
							closeConnect(connect_socket);
							break;
						}
						else if (a == 3)
						{ //������ʱ��
							getTime(send_buf, connect_socket);
							continue;
						}
						else if (a == 4)
						{ //�����÷���������
							getName(send_buf, connect_socket);
							continue;
						}
						else if (a == 5)
						{ //�����ÿͻ����б�
							getList(send_buf, connect_socket);
							continue;
						}
						else if (a == 6)
						{ //�����������ͻ��˷�����Ϣ
							sendMessage(send_buf, connect_socket);
							continue;
						}
						else if (a == 7)
						{//�����˳�
							dropout(connect_socket);
							return 0;
						}
						else
						{
							continue;
						}
					}
					continue;
				}
			}
		}

		else
		{ //�������δ���ӷ����ʱ����ͼ��������
			cout << "You haven't connect to a server!" << endl;
			continue;
		}
	}
}

void helpinfo()
{
	puts("Please input the operation number:\n"
		"+-------+--------------------------------------+\n"
		"| Input |              Function                |\n"
		"+-------+--------------------------------------+\n"
		"|   1   |  Connect to a server.                |\n"
		"|   2   |  Close the connect.                  |\n"
		"|   3   |  Get the server time.                |\n"
		"|   4   |  Get the server name.                |\n"
		"|   5   |  Get the client list.                |\n"
		"|   6   |  Send a message.                     |\n"
		"|   7   |  Exit.                               |\n"
		"+-------+--------------------------------------+\n");
}

void output(string response_content)
{
	mtx_output.lock(); //�Ի���������
	cout << response_content << endl;
	mtx_output.unlock(); //�Ի���������
}

int receivePacket(SOCKET connect_socket)
{
	char recv_buf[DEFAULT_BUFLEN];
	int result;
	int recv_buflen = DEFAULT_BUFLEN;
	do
	{
		ZeroMemory(recv_buf, DEFAULT_BUFLEN);
		result = recv(connect_socket, recv_buf, recv_buflen, 0); //�������ݣ����յ������ݰ�����recv_buf��
		if (result > 0)                                          //���ճɹ������ؽ��յ������ݳ���
		{
			string recv_packet = recv_buf; //���յ������ݰ�
			char packet_type;              //���յ������ݰ�����
			string response_content;       //���յ������ݰ�����

			int pos1 = 0;
			int pos2 = 0;

			//���յ������ݰ�����
			pos1 = recv_packet.find("#") + 1; //���յ������ݰ����͵�λ�ã�start��
			packet_type = recv_packet[pos1];  //���յ������ݰ�����

			//���յ������ݰ�����
			pos1 = recv_packet.find("*") + 1;   //�ҵ��������ش��λ�ã�start��
			pos2 = recv_packet.find("#", pos1); //��pos1����λ�ÿ�ʼ�����ң��ҵ��ո�Ϊֹ
			if ((pos2 - pos1) == 0)             //����Ϊ��
				response_content = "";
			else //���ݲ�Ϊ��
				response_content = recv_packet.substr(pos1, pos2 - pos1);

			switch (packet_type)
			{
			case 'T': //���ݰ�����Ϊʱ��
				output(response_content);
				Time.notify_one();
				break;
			case 'N': //���ݰ�����Ϊ����
				output(response_content);
				;
				name.notify_one();
				break;
			case 'L': //���ݰ�����Ϊ�ͻ����б�
				output(response_content);
				list.notify_one();
				break;
			case 'M': //���ݰ�����Ϊ��Ϣ
				pos1 = recv_packet.find("-") + 1;
				char type = recv_packet[pos1];
				switch (type)
				{
				case 'Y': //��ʾ������Ϣ�ɹ�
					//������Ϣ��һ���ı��
					int request_ID;
					pos1 = recv_packet.find("*") + 1;
					pos2 = recv_packet.find(" ", pos1);
					request_ID = stoi(recv_packet.substr(pos1, pos2 - pos1)); //���ַ���ת��Ϊ����
					response_content = "the message has been sent to ";
					response_content += to_string(request_ID);
					output(response_content);
					message.notify_one();
					break;
				case 'N': //��ʾ������Ϣʧ��
					output(response_content);
					message.notify_one();
					break;
				case 'M': //��ʾ�յ���Ϣ
					//������Ϣ�Ŀͻ���ID
					int from_ID;
					pos1 = recv_packet.find("*") + 1;
					pos2 = recv_packet.find(" ", pos1);
					from_ID = stoi(recv_packet.substr(pos1, pos2 - pos1));

					//������Ϣ�Ŀͻ���IP
					string from_IP;
					pos1 = pos2 + 1;
					pos2 = recv_packet.find("\n");
					from_IP = recv_packet.substr(pos1, pos2 - pos1);

					//�յ�����Ϣ
					string receive_message;
					pos1 = pos2 + 1;
					pos2 = recv_packet.find("#", pos1);
					receive_message = recv_packet.substr(pos1, pos2 - pos1);

					mtx_output.lock();
					cout << "Message from " << to_string(from_ID) << " " << from_IP << endl;
					cout << receive_message << endl;
					mtx_output.unlock();
					break;
				}
			}
		}
		else if (result == 0) //���ӽ���
			printf("Connection closed\n");
		else //����ʧ��
			printf("recv failed with error: %d\n", WSAGetLastError());

	} while (result > 0);
	return 0;
}

void closeConnect(SOCKET connect_socket)
{
	int ret;
	ret = shutdown(connect_socket, SD_SEND);
	if (ret == SOCKET_ERROR)
	{
		cout << "shutdown failed with error: " << to_string(WSAGetLastError()) << endl;
		closesocket(connect_socket);
		WSACleanup();
	}
}

void getTime(char* send_buf, SOCKET connect_socket)
{
	int ret;
	string packet = "#T#";
	memcpy(send_buf, packet.c_str(), packet.size());
	//for (int i = 1; i <= 100; i++) {
		ret = send(connect_socket, send_buf, (int)strlen(send_buf), 0);
		unique_lock<mutex> lck(mtx_time);
		Time.wait(lck);
	//}
	
}

void getName(char* send_buf, SOCKET connect_socket)
{
	int ret;
	string packet = "#N#";
	memcpy(send_buf, packet.c_str(), packet.size());
	ret = send(connect_socket, send_buf, (int)strlen(send_buf), 0);
	unique_lock<mutex> lck(mtx_name);
	name.wait(lck);
}

void getList(char* send_buf, SOCKET connect_socket)
{
	int ret;
	string packet = "#L#";
	memcpy(send_buf, packet.c_str(), packet.size());
	ret = send(connect_socket, send_buf, (int)strlen(send_buf), 0);
	unique_lock<mutex> lck(mtx_list);
	list.wait(lck);
}

void sendMessage(char* send_buf, SOCKET connect_socket)
{
	string sendMessage;
	char buffer[1024] = "\0";
	int dst_ID = 0;
	int ret;

	//�ͻ��˵�ID
	cout << "please input the destination id:" << endl;
	cin >> dst_ID;
	cin.getline(buffer, 1024);

	//Ҫ���͵���Ϣ����
	cout << "please input the message:" << endl;
	cin.getline(buffer, 1024);
	sendMessage = buffer;

	//���͵����ݰ�
	string packet = "#M*";
	packet += to_string(dst_ID);
	packet += "*";
	packet += sendMessage;
	packet += "#";
	ZeroMemory(send_buf, DEFAULT_BUFLEN);
	memcpy(send_buf, packet.c_str(), packet.size());
	ret = send(connect_socket, send_buf, (int)strlen(send_buf), 0);
	unique_lock<mutex> lck(mtx_message);
	message.wait(lck);
}

void dropout(SOCKET connect_socket)
{
	int ret;
	ret = shutdown(connect_socket, SD_SEND);
	if (ret == SOCKET_ERROR)
	{
		cout << "shutdown failed with error: " << to_string(WSAGetLastError()) << endl;
		closesocket(connect_socket);
		WSACleanup();
	}
}