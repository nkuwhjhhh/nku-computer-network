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

#define SEND_MAX 0x3A00
#define SEND_TOP 0x1C

char sendBuf[SEND_MAX];
char recvBuf[SEND_MAX];
char content[10000][SEND_MAX - SEND_TOP];//���������ȴ���������
char name[100];
int sum = 65535;//�ļ����ĸ���
u_int endfilesize = 0;
int seq = 0;

char IPServer[] = "127.0.0.1";
char IPClient[] = "127.0.0.1";
u_short PortServer = 4000;
u_short PortClient = 5678;

//�ж��ǲ����ļ��ĵ�һ�����ģ��ǣ��½�һ���ļ�����ʼ�����ݷ��룩
//�ж��ǲ����ļ����һ�����ģ��ǣ����filelength�еĳ��ȣ���index���ļ����뿿������ȣ�
//��������ǣ���index�������ݷ���

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
//����У��Ͳ�����
void calculatechecksum()
{
	int sum = 0;
	//����ǰ26�ֽ�
	for (int i = 0; i < 13; i += 2)
	{
		sum += (sendBuf[2 * i] << 8) + sendBuf[2 * i + 1];
		//���ֽ�λ
		if (sum >= 0x10000)
		{
			sum -= 0x10000;
			sum += 1;
		}
	}
	//checksum����
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
		//cout << "�ɹ�У��" << endl;
		return true;
	}
	{
		//cout << "ʧ��У��" << endl;
		return false;
	}
}

int calculateindex()
{
	return (((unsigned int)((unsigned char)recvBuf[24]) << 8)
		+ ((unsigned int)(unsigned char)recvBuf[25]));
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

void setcontent(int index)
{
	for (int i = 0; i < SEND_MAX - SEND_TOP; i++)
	{
		content[index - 1][i] = recvBuf[i + 28];
	}
}
void setcontentend(int index)
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

void getseqsetack()
{
	sendBuf[18] = recvBuf[16];
	sendBuf[19] = recvBuf[17];
}

void outfile(int index)
{
	ofstream fout(name, ofstream::binary);
	for (int i = 0; i < index - 1; i++)
	{
		for (int j = 0; j < (SEND_MAX - SEND_TOP); j++)
			fout << content[i][j];
	}
	for (int j = 0; j < endfilesize; j++)
		fout << content[index - 1][j];
}

void printsend()
{
	cout << "-------------------------------------------------" << endl;
	cout << "     ����     " << endl;
	cout << "ACK     " << (u_short)sendBuf[0] << endl;
	cout << "SYN     " << (u_short)sendBuf[1] << endl;
	cout << "FIN     " << (u_short)sendBuf[2] << endl;
	cout << "SFEF    " << (u_short)sendBuf[3] << endl;
	cout << "seq��   " << (u_short)sendBuf[16] << endl;
	cout << "seq��   " << (u_short)sendBuf[17] << endl;
	cout << "ack��   " << (u_short)sendBuf[18] << endl;
	cout << "ack��   " << (u_short)sendBuf[19] << endl;
}
void printrecv()
{
	cout << "-------------------------------------------------" << endl;
	cout << "     ����     " << endl;
	cout << "ACK     " << (u_short)recvBuf[0] << endl;
	cout << "SYN     " << (u_short)recvBuf[1] << endl;
	cout << "FIN     " << (u_short)recvBuf[2] << endl;
	cout << "SFEF    " << (u_short)recvBuf[3] << endl;
	cout << "seq��   " << (u_short)recvBuf[16] << endl;
	cout << "seq��   " << (u_short)recvBuf[17] << endl;
	cout << "ack��   " << (u_short)recvBuf[18] << endl;
	cout << "ack��   " << (u_short)recvBuf[19] << endl;
}

void main()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(1, 1);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)return;
	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)      //���ֽ�Ϊ���汾
	{
		WSACleanup();
		return;
	}
	printf("server is operating!\n\n");
	SOCKET sockSrv = socket(AF_INET, SOCK_DGRAM, 0);
	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.s_addr = inet_addr(IPServer);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(PortServer);
	bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
	SOCKADDR_IN addrClient;   //�������տͻ��˵ĵ�ַ��Ϣ
	int len = sizeof(SOCKADDR);
	//int time_out = 1;//1ms��ʱ
	//setsockopt(sockSrv, SOL_SOCKET, SO_RCVTIMEO, (char*)&time_out, sizeof(time_out));
	setIPandPort();
	int size = 0;
	//�������ӣ��������ݣ��ж�����У��ͣ���Ӧ����
	while (1)
	{
		if (recvfrom(sockSrv, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&addrClient, &len) != -1)
		{
			cout << "��������" << endl;
			//printrecv();
			if (verifychecksum())
			{
				cout << "���ӳɹ�" << endl;
				sendBuf[0] = 1;
				getseqsetack();
				calculatechecksum();
				sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
				//printsend();
				break;
			}
			else
			{
				//cout << "����ACK=0" << endl;
				sendBuf[0] = 0;
				getseqsetack();
				calculatechecksum();
				sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
				//printsend();
			}
		}
	}
	//�������ݱ���
	int recvseq = (((unsigned int)((unsigned char)recvBuf[16]) << 8))
		+ (((unsigned int)(unsigned char)recvBuf[17]));
	int thisseq = recvseq;
	while (1)
	{
		if (recvfrom(sockSrv, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&addrClient, &len) != -1)
		{
			if (recvBuf[2] == 1)break;
			thisseq = (((unsigned int)((unsigned char)recvBuf[16]) << 8))
				+ (((unsigned int)(unsigned char)recvBuf[17]));
			if (verifychecksum())
			{
				if (recvseq == thisseq - 1)
				{//����ϴ��յ��ĵ�������յ�-1����˳�����
					//����ϴ��յ��Ĵ�������յ���-1����ظ���Ӧ��
					if (recvBuf[3] == 0x10)
					{
						setname();
					}
					if (recvBuf[3] == 0x00)
					{
						setcontent(calculateindex());
					}
					if (recvBuf[3] == 0x01)
					{
						size = calculateindex();
						setcontentend(size);
					}
					sendBuf[0] = 1;//ACK=1
					getseqsetack();
					calculatechecksum();
					recvseq = thisseq;
					sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
					//cout << "111111----------------------------------------" << endl;
					//printsend();
				}
				else
				{
					if (recvseq > thisseq - 1)
					{
						sendBuf[0] = 1;//ACK=1
						getseqsetack();
						calculatechecksum();
						sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
						//cout << "222222----------------------------------------" << endl;
						//printsend();
					}
					else
					{
						sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
						//cout << "333333----------------------------------------" << endl;
						//printsend();
					}
				}
			}
			else
			{
				sendBuf[0] = 0;//ack=0
				calculatechecksum();
				sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
				printsend();
			}
		}
	}
	//�Ͽ�����
	while (1)
	{
		if (recvfrom(sockSrv, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&addrClient, &len) != -1)
		{
			cout << "Ҫ�Ͽ���" << endl;
			cout << (u_short)recvBuf[2] << endl;
			//printrecv();
			if ((recvBuf[2] == 1) && (verifychecksum()))
			{
				cout << "�ɹ��Ͽ�" << endl;
				sendBuf[0] = 1;//ack=1
				getseqsetack();
				calculatechecksum();
				for (int i = 0; i < 2; i++)
					sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
				//printsend();
				break;
			}
			else
			{
				sendBuf[0] = 0;//ack=0
				getseqsetack();
				calculatechecksum();
				//printsend();
				sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrClient, len);
			}
		}
	}
	cout << "Ҫд��" << endl;
	//cout << size << endl;
	outfile(size);
	closesocket(sockSrv);
	WSACleanup();
}
