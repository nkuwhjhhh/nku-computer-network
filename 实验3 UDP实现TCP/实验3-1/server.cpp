#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#include<iostream>
#include <winsock2.h>
#include<time.h>
#include <Winsock2.h>
#include <stdio.h>
#include<fstream>
#include <string>
#include <vector>
#include <io.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

#define SEND_MAX 0x4000
#define SEND_TOP 0x1C

char sendBuf[SEND_MAX];
char recvBuf[SEND_MAX];
char content[10000][16356];//传输内容先存在数组里
char name[100];
int sum = 65535;//文件报文个数
int index = 0;//设置读入文件的第几个段
u_int endfilesize = 0;
int seq = 0;

char IPServer[] = "172.19.0.1";
char IPClient[] = "127.0.0.2";
u_short PortServer = 1234;
u_short PortClient = 5678;

//判断是不是文件的第一个报文（是，新建一个文件，开始将内容放入）
//判断是不是文件最后一个报文（是，检测filelength中的长度，看index，文件读入靠这个长度）
//如果都不是，看index，将内容放入

void setIPandPort()
{
	int x = 4;
	u_int a = 0;
	for (int i = 0; IPServer[i] != '\0'; i++)
	{
		if (IPServer[i] == '.')
		{
			sendBuf[x++] = (char)a;
			a = 0;
			continue;
		}
		a = a * 10 + IPServer[i] - 48;
	}
	sendBuf[x++] = (char)a;
	a = 0;
	for (int i = 0; IPClient[i] != '\0'; i++)
	{
		if (IPClient[i] == '.')
		{
			sendBuf[x++] = (char)(u_short)a;
			a = 0;
			continue;
		}
		a = a * 10 + IPClient[i] - 48;
	}
	sendBuf[x++] = (char)a;
	sendBuf[12] = (char)(PortServer >> 8);
	sendBuf[13] = (char)PortServer;
	sendBuf[14] = (char)(PortClient >> 8);
	sendBuf[15] = (char)PortClient;
}

u_short calchecksum;
//计算校验和并填入
void calculatechecksum()
{
	int sum = 0;
	//计算前26字节
	for (int i = 0; i < 13; i += 2)
	{
		sum += (sendBuf[2 * i] << 8) + sendBuf[2 * i + 1];
		//出现进位
		if (sum >= 0x10000)
		{
			sum -= 0x10000;
			sum += 1;
		}
	}
	//checksum计算
	calchecksum = ~(u_short)sum;
	sendBuf[26] = (char)(calchecksum >> 8);
	sendBuf[27] = (char)calchecksum;
}
u_short checksum;
bool verifychecksum()
{
	int sum = 0;
	for (int i = 0; i < 13; i += 2)
	{
		sum += (recvBuf[2 * i] << 8) + recvBuf[2 * i + 1];
		if (sum >= 0x10000)
		{
			sum -= 0x10000;
			sum += 1;
		}
	}
	checksum = (recvBuf[26] << 8) + (unsigned char)recvBuf[27];
	if (checksum + (u_short)sum == 0xffff)
	{
		cout << "成功校验" << endl;
		return true;
	}
	{
		cout << "失败校验" << endl;
		return false;
	}
}

int sizeofname = 0;
void setname()
{
	sizeofname = (((unsigned int)((unsigned char)recvBuf[20]) << 24))
		+ (((unsigned int)((unsigned char)recvBuf[21]) << 16))
		+ (((unsigned int)((unsigned char)recvBuf[22]) << 8))
		+ (((unsigned int)(unsigned char)recvBuf[23]));
	for (int i = 0; i < sizeofname; i++)
		name[i] = recvBuf[i + 28];
}

void setcontent()
{
	for (int i = 0; i < SEND_MAX-SEND_TOP; i++)
	{
		content[index-1][i] = recvBuf[i + 28];
	}
}
void setcontentend()
{
	endfilesize = (((unsigned int)((unsigned char)recvBuf[20]) << 24))
		+ (((unsigned int)((unsigned char)recvBuf[21]) << 16))
		+ (((unsigned int)((unsigned char)recvBuf[22]) << 8))
		+ (((unsigned int)(unsigned char)recvBuf[23]));
	for (int i = 0; i < endfilesize; i++)
	{
		content[index-1][i]= recvBuf[i + 28];
	}
}

void get_and_set_seq()
{
	seq = 0;
	for (int i = 16; i < 20; i++)
	{
		sendBuf[i] = recvBuf[i];
	}
	seq = (((unsigned int)((unsigned char)recvBuf[16]) << 24))
		+ (((unsigned int)((unsigned char)recvBuf[17]) << 16))
		+ (((unsigned int)((unsigned char)recvBuf[18]) << 8))
		+ (((unsigned int)(unsigned char)recvBuf[19]));
}

void outfile()
{
	ofstream fout(name, ofstream::binary);
	for (int i = 0; i < index-1; i++)
	{
		for (int j = 0; j < 0x3FE4; j++)
			fout << content[i][j];
	}
	for (int j = 0; j < endfilesize; j++)
		fout << content[index-1][j];
}

void getlog1(int a)
{
	if (a == 1)
	{
		cout <<"send"
			<< "      seq " << (int)seq
			<< "      ACK " << (int)sendBuf[0]
			<< "      SYN " << (int)sendBuf[1]
			<< "      FIN " << (int)sendBuf[2]
			<< "      校验和 " << (int)calchecksum
			<< endl;
	}
	if (a == 0)
	{
		cout << "recv"
			<< "      seq " << (int)seq
			<< "      ACK " << (int)recvBuf[0]
			<< "      SYN " << (int)recvBuf[1]
			<< "      FIN " << (int)recvBuf[2]
			<< "      校验和 " << (int)checksum
			<< endl;
	}
}
void getlog()
{
	cout << "send" 
		<< "      seq " << (int)seq
		<< "      ACK " << (int)sendBuf[0]
		<< "      SYN " << (int)sendBuf[1]
		<< "      FIN " << (int)sendBuf[2]
		<< "      校验和 " << (int)calchecksum
		<< "      index " << (int)index;
	if (index == sum)
	{
		cout << "      length " << (int)endfilesize << endl;
	}
	else
	{
		cout << "       length " << (int)16356 << endl;
	}
}

void main()
{
	//加载套接字库
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(1, 1);

	err = WSAStartup(wVersionRequested, &wsaData);//错误会返回WSASYSNOTREADY
	if (err != 0)
	{
		return;
	}

	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)      //高字节为副版本
	{
		WSACleanup();
		return;
	}

	printf("server is operating!\n\n");
	//创建用于监听的socket
	SOCKET sockSrv = socket(AF_INET, SOCK_DGRAM, 0);//失败会返回 INVALID_SOCKET
	
	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.s_addr = inet_addr(IPServer);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(PortServer);


	//绑定套接字, 绑定到端口
	bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));//会返回一个SOCKET_ERROR


	SOCKADDR_IN addrClient;   //用来接收客户端的地址信息
	int len = sizeof(SOCKADDR);

	setIPandPort();

	//建立连接，接收数据，判断数据校验和，回应报文
	while (1)
	{
		recvfrom(sockSrv, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&addrClient, &len);
		if (verifychecksum())
		{
			getlog1(0);
			sendBuf[0] = 1;
			get_and_set_seq();
			calculatechecksum();
			getlog1(1);
			sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
			break;
		}
		else
		{
			getlog1(0);
			sendBuf[0] = 0;
			get_and_set_seq();
			calculatechecksum();
			getlog1(1);
			sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
		}
	}


	//接收数据报文
	while (index!=sum)
	{
		//判断recv[3]，为0x10时，接收数据，根据index将内容放入相应的content数组
		recvfrom(sockSrv, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&addrClient, &len);
		getlog1(0);
		if (verifychecksum())
		{
			if (recvBuf[3] == 0x1)
			{
				sum = index + 1;
				index++;
				setcontentend();
			}
			if (recvBuf[3] == 0x10)
			{
				index = 0;
				setname();
			}
			if (recvBuf[3] == 0x0)
			{
				index++;
				setcontent();
			}

			sendBuf[0] = 1;//ack=1
			get_and_set_seq();
			calculatechecksum();
			getlog1(1);
			sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
		}
		else
		{
			getlog1(0);
			sendBuf[0] = 0;//ack=0
			get_and_set_seq();
			calculatechecksum();
			getlog1(1);
			sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
		}
	}
	//断开连接
	while (1)
	{
		recvfrom(sockSrv, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&addrClient, &len);
		getlog1(0);
		if ((recvBuf[2] == 1)&&(verifychecksum()))
		{
				sendBuf[0] = 1;//ack=1
				get_and_set_seq();
				calculatechecksum();
				getlog1(1);
				sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
				break;
			
		}
		else
		{
			sendBuf[0] = 0;//ack=0
			get_and_set_seq();
			calculatechecksum();
			getlog1(1);
			sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
		}
	}
	outfile();
	closesocket(sockSrv);
	WSACleanup();
}







//只跑程序

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#include<iostream>
#include <winsock2.h>
#include<time.h>
#include <Winsock2.h>
#include <stdio.h>
#include<fstream>
#include <string>
#include <vector>
#include <io.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

#define SEND_MAX 0x4000
#define SEND_TOP 0x1C

char sendBuf[SEND_MAX];
char recvBuf[SEND_MAX];
char content[10000][16356];//传输内容先存在数组里
char name[100];
int sum = 65535;//文件报文个数
int index = 0;//设置读入文件的第几个段
u_int endfilesize = 0;
int seq = 0;

char IPServer[] = "172.19.0.1";
char IPClient[] = "127.0.0.2";
u_short PortServer = 1234;
u_short PortClient = 5678;

//判断是不是文件的第一个报文（是，新建一个文件，开始将内容放入）
//判断是不是文件最后一个报文（是，检测filelength中的长度，看index，文件读入靠这个长度）
//如果都不是，看index，将内容放入

void setIPandPort()
{
	int x = 4;
	u_int a = 0;
	for (int i = 0; IPServer[i] != '\0'; i++)
	{
		if (IPServer[i] == '.')
		{
			sendBuf[x++] = (char)a;
			a = 0;
			continue;
		}
		a = a * 10 + IPServer[i] - 48;
	}
	sendBuf[x++] = (char)a;
	a = 0;
	for (int i = 0; IPClient[i] != '\0'; i++)
	{
		if (IPClient[i] == '.')
		{
			sendBuf[x++] = (char)(u_short)a;
			a = 0;
			continue;
		}
		a = a * 10 + IPClient[i] - 48;
	}
	sendBuf[x++] = (char)a;
	sendBuf[12] = (char)(PortServer >> 8);
	sendBuf[13] = (char)PortServer;
	sendBuf[14] = (char)(PortClient >> 8);
	sendBuf[15] = (char)PortClient;
}

u_short calchecksum;
//计算校验和并填入
void calculatechecksum()
{
	int sum = 0;
	//计算前26字节
	for (int i = 0; i < 13; i += 2)
	{
		sum += (sendBuf[2 * i] << 8) + sendBuf[2 * i + 1];
		//出现进位
		if (sum >= 0x10000)
		{
			sum -= 0x10000;
			sum += 1;
		}
	}
	//checksum计算
	calchecksum = ~(u_short)sum;
	sendBuf[26] = (char)(calchecksum >> 8);
	sendBuf[27] = (char)calchecksum;
}
u_short checksum;
bool verifychecksum()
{
	int sum = 0;
	for (int i = 0; i < 13; i += 2)
	{
		sum += (recvBuf[2 * i] << 8) + recvBuf[2 * i + 1];
		if (sum >= 0x10000)
		{
			sum -= 0x10000;
			sum += 1;
		}
	}
	checksum = (recvBuf[26] << 8) + (unsigned char)recvBuf[27];
	if (checksum + (u_short)sum == 0xffff)
	{
		//cout << "成功校验" << endl;
		return true;
	}
	{
		//cout << "失败校验" << endl;
		return false;
	}
}

int sizeofname = 0;
void setname()
{
	sizeofname = (((unsigned int)((unsigned char)recvBuf[20]) << 24))
		+ (((unsigned int)((unsigned char)recvBuf[21]) << 16))
		+ (((unsigned int)((unsigned char)recvBuf[22]) << 8))
		+ (((unsigned int)(unsigned char)recvBuf[23]));
	for (int i = 0; i < sizeofname; i++)
		name[i] = recvBuf[i + 28];
}

void setcontent()
{
	for (int i = 0; i < SEND_MAX - SEND_TOP; i++)
	{
		content[index - 1][i] = recvBuf[i + 28];
	}
}
void setcontentend()
{
	endfilesize = (((unsigned int)((unsigned char)recvBuf[20]) << 24))
		+ (((unsigned int)((unsigned char)recvBuf[21]) << 16))
		+ (((unsigned int)((unsigned char)recvBuf[22]) << 8))
		+ (((unsigned int)(unsigned char)recvBuf[23]));
	for (int i = 0; i < endfilesize; i++)
	{
		content[index - 1][i] = recvBuf[i + 28];
	}
}

void get_and_set_seq()
{
	seq = 0;
	for (int i = 16; i < 20; i++)
	{
		sendBuf[i] = recvBuf[i];
	}
	seq = (((unsigned int)((unsigned char)recvBuf[16]) << 24))
		+ (((unsigned int)((unsigned char)recvBuf[17]) << 16))
		+ (((unsigned int)((unsigned char)recvBuf[18]) << 8))
		+ (((unsigned int)(unsigned char)recvBuf[19]));
}

void outfile()
{
	ofstream fout(name, ofstream::binary);
	for (int i = 0; i < index - 1; i++)
	{
		for (int j = 0; j < 0x3FE4; j++)
			fout << content[i][j];
	}
	for (int j = 0; j < endfilesize; j++)
		fout << content[index - 1][j];
}

void getlog1(int a)
{
	if (a == 1)
	{
		cout << "send"
			<< "      seq " << (int)seq
			<< "      ACK " << (int)sendBuf[0]
			<< "      SYN " << (int)sendBuf[1]
			<< "      FIN " << (int)sendBuf[2]
			<< "      校验和 " << (int)calchecksum
			<< endl;
	}
	if (a == 0)
	{
		cout << "recv"
			<< "      seq " << (int)seq
			<< "      ACK " << (int)recvBuf[0]
			<< "      SYN " << (int)recvBuf[1]
			<< "      FIN " << (int)recvBuf[2]
			<< "      校验和 " << (int)checksum
			<< endl;
	}
}
void getlog()
{
	cout << "send"
		<< "      seq " << (int)seq
		<< "      ACK " << (int)sendBuf[0]
		<< "      SYN " << (int)sendBuf[1]
		<< "      FIN " << (int)sendBuf[2]
		<< "      校验和 " << (int)calchecksum
		<< "      index " << (int)index;
	if (index == sum)
	{
		cout << "      length " << (int)endfilesize << endl;
	}
	else
	{
		cout << "       length " << (int)16356 << endl;
	}
}

void main()
{
	//加载套接字库
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(1, 1);

	err = WSAStartup(wVersionRequested, &wsaData);//错误会返回WSASYSNOTREADY
	if (err != 0)
	{
		return;
	}

	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)      //高字节为副版本
	{
		WSACleanup();
		return;
	}

	printf("server is operating!\n\n");
	//创建用于监听的socket
	SOCKET sockSrv = socket(AF_INET, SOCK_DGRAM, 0);//失败会返回 INVALID_SOCKET

	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.s_addr = inet_addr(IPServer);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(PortServer);


	//绑定套接字, 绑定到端口
	bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));//会返回一个SOCKET_ERROR


	SOCKADDR_IN addrClient;   //用来接收客户端的地址信息
	int len = sizeof(SOCKADDR);

	setIPandPort();

	//建立连接，接收数据，判断数据校验和，回应报文
	while (1)
	{
		recvfrom(sockSrv, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&addrClient, &len);
		if (verifychecksum())
		{
			//getlog1(0);
			sendBuf[0] = 1;
			get_and_set_seq();
			calculatechecksum();
			//getlog1(1);
			sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
			break;
		}
		else
		{
			//getlog1(0);
			sendBuf[0] = 0;
			get_and_set_seq();
			calculatechecksum();
			//getlog1(1);
			sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
		}
	}


	//接收数据报文
	while (index != sum)
	{
		//判断recv[3]，为0x10时，接收数据，根据index将内容放入相应的content数组
		recvfrom(sockSrv, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&addrClient, &len);
		//getlog1(0);
		if (verifychecksum())
		{
			if (recvBuf[3] == 0x1)
			{
				sum = index + 1;
				index++;
				setcontentend();
			}
			if (recvBuf[3] == 0x10)
			{
				index = 0;
				setname();
			}
			if (recvBuf[3] == 0x0)
			{
				index++;
				setcontent();
			}

			sendBuf[0] = 1;//ack=1
			get_and_set_seq();
			calculatechecksum();
			//getlog1(1);
			sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
		}
		else
		{
			//getlog1(0);
			sendBuf[0] = 0;//ack=0
			get_and_set_seq();
			calculatechecksum();
			//getlog1(1);
			sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
		}
	}
	//断开连接
	while (1)
	{
		recvfrom(sockSrv, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&addrClient, &len);
		//getlog1(0);
		if ((recvBuf[2] == 1) && (verifychecksum()))
		{
			sendBuf[0] = 1;//ack=1
			get_and_set_seq();
			calculatechecksum();
			//getlog1(1);
			sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
			break;

		}
		else
		{
			sendBuf[0] = 0;//ack=0
			get_and_set_seq();
			calculatechecksum();
			//getlog1(1);
			sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
		}
	}
	outfile();
	closesocket(sockSrv);
	WSACleanup();
}