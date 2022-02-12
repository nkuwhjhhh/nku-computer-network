#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#include<iostream>
#include<fstream>
#include <winsock2.h>
#include<time.h>
#include <Winsock2.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <queue>
#include <io.h>
#include <thread>
#define SEND_MAX 0x4000
#define SEND_TOP 0x1C
#define WINDOW_SIZE 10//窗口大小为5
#define TIMEOUT 1000//超时
#pragma comment(lib, "ws2_32.lib")
using namespace std;

char sendBuf[SEND_MAX];
char recvBuf[SEND_MAX];
char content[10000][16356];//传输内容先存在数组里
char name[100];
int sum = 0;//文件报文个数
//int index = 0;//设置读入文件的第几个段
int endfilesize = 0;
int seq = 0;

SOCKADDR_IN  addrServer;
int len = sizeof(SOCKADDR);

char IPServer[] = "172.19.0.1";
char IPClient[] = "127.0.0.2";
u_short PortServer = 1234;
u_short PortClient = 5678;

//检测所有的文件
bool getFiles(string path, vector<string>& files)
{
	//文件句柄
	long hFile = 0;
	//文件信息
	struct _finddata_t fileinfo;
	string p;
	if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			files.push_back(p.assign(path).append("\\").append(fileinfo.name));
		} while (_findnext(hFile, &fileinfo) == 0);
		_findclose(hFile);
	}
	return 1;
}

//输出所有检测到的文件并输出，提供选择，装入分组
void log()
{
	//下面开始检测文件夹下的测试文件
	cout << "下面开始检测任务1测试文件" << endl;
	vector<string> files;
	string path("C:\\web\\task_one_test_file");
	if (getFiles(path, files))
		cout << "读文件成功" << endl;
	int num = files.size();
	for (int i = 0; i < num; i++)cout << files[i].c_str() << endl;
	int choose = 0;
	cout << "输入文件序号" << endl;
	cin >> choose;
	cout << "文件名选择文件" << endl;
	cin >> name;
	ifstream fin(files[choose].c_str(), ifstream::binary);
	char t = fin.get();
	int length = 0;
	while (fin)
	{
		content[sum][length % 16356] = t;
		length++;
		if (length % 16356 == 0)
		{
			sum++;
			length = 0;
		}
		t = fin.get();
	}
	endfilesize = length;
	if (endfilesize)sum++;
	cout << "文件报文个数" << sum << endl;
	cout << "最后一个文件报文大小" << endfilesize << endl;
}

//设置报文IP和端口号
void setIPandPort()
{
	int x = 4;
	u_int a = 0;
	for (int i = 0; IPClient[i] != '\0'; i++)
	{
		if (IPClient[i] == '.')
		{
			sendBuf[x++] = (char)a;
			a = 0;
			continue;
		}
		a = a * 10 + IPClient[i] - 48;
	}
	sendBuf[x++] = (char)a;
	a = 0;
	for (int i = 0; IPServer[i] != '\0'; i++)
	{
		if (IPServer[i] == '.')
		{
			sendBuf[x++] = (char)(u_short)a;
			a = 0;
			continue;
		}
		a = a * 10 + IPServer[i] - 48;
	}
	sendBuf[x++] = (char)a;
	sendBuf[12] = (char)(PortClient >> 8);
	sendBuf[13] = (char)PortClient;
	sendBuf[14] = (char)(PortServer >> 8);
	sendBuf[15] = (char)PortServer;
}

u_short calchecksum;
void calculatechecksum()
{
	int sum = 0;
	//计算前26字节
	for (int i = 0; i < 13; i += 2)
	{
		sum += (sendBuf[2 * i] << 8) + sendBuf[2 * i + 1];
		if (sum >= 0x10000)
		{
			sum -= 0x10000;
			sum += 1;
		}
	}
	calchecksum = ~(u_short)sum;
	sendBuf[26] = (char)(calchecksum >> 8);
	sendBuf[27] = (char)calchecksum;
}

u_short checksum;
bool verifychecksum()
{
	int sum = 0;
	//计算前26字节
	for (int i = 0; i < 13; i += 2)
	{
		sum += (recvBuf[2 * i] << 8) + recvBuf[2 * i + 1];
		//出现进位
		if (sum >= 0x10000)
		{
			sum -= 0x10000;
			sum += 1;
		}
	}
	//checksum计算
	//取反
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

void want_to_connect()
{
	for (int i = 0; i < 4; i++)
		sendBuf[i] = 0;
	//syn设为1其余设置为0
	sendBuf[1] = 1;
	sendBuf[20] = sendBuf[21] = sendBuf[22] = sendBuf[23] = sendBuf[24] = sendBuf[25] = 0;
	sendBuf[16] = (char)(seq >> 8);
	sendBuf[17] = (char)(seq);
	calculatechecksum();
}


void printsend()
{
	cout << "-----------------------------" << endl;
	cout << "     发送     " << endl;
	cout << "ACK     " << (u_short)sendBuf[0] << endl;
	cout << "SYN     " << (u_short)sendBuf[1] << endl;
	cout << "FIN     " << (u_short)sendBuf[2] << endl;
	cout << "SFEF    " << (u_short)sendBuf[3] << endl;
	cout << "seq左   " << (u_short)(u_char)sendBuf[16] << endl;
	cout << "seq右   " << (u_short)(u_char)sendBuf[17] << endl;
	cout << "ack左   " << (u_short)(u_char)sendBuf[18] << endl;
	cout << "ack右   " << (u_short)(u_char)sendBuf[19] << endl;
}

void printrecv()
{
	cout << "-----------------------------" << endl;
	cout << "     接收     " << endl;
	cout << "ACK     " << (u_short)recvBuf[0] << endl;
	cout << "SYN     " << (u_short)recvBuf[1] << endl;
	cout << "FIN     " << (u_short)recvBuf[2] << endl;
	cout << "SFEF    " << (u_short)recvBuf[3] << endl;
	cout << "seq左   " << (u_short)(u_char)recvBuf[16] << endl;
	cout << "seq右   " << (u_short)(u_char)recvBuf[17] << endl;
	cout << "ack左   " << (u_short)(u_char)recvBuf[18] << endl;
	cout << "ack右   " << (u_short)(u_char)recvBuf[19] << endl;
}


//对最后一个设置结束标志，同时将结束位置放进去
//参数为0到sum
//设置此处seq为seq+参数
void want_to_sendfile(int want_to_send)
{
	int index = want_to_send - seq;
	for (int i = 0; i < 4; i++)
		sendBuf[i] = 0;
	//对每一个都设置index
	sendBuf[24] = (char)(index >> 8);
	sendBuf[25] = (char)index;

	//将filelength所在初始化为0
	for (int i = 20; i < 24; i++) sendBuf[i] = 0;

	//最后一个设置filelength还有多少字节
	if (index == sum)
	{
		cout << "最后一个装填进来了" << endl;
		for (int j = 0; j < endfilesize; j++)
			sendBuf[j + 28] = content[index - 1][j];
		for (int j = 0; j < 4; j++)
			sendBuf[20 + j] = (char)(endfilesize >> (24 - 8 * j));
		sendBuf[3] = 0x1;
	}
	else
	{
		if (index == 0)
		{
			for (int j = 0; j < sizeof(name); j++)
			{
				sendBuf[j + 28] = name[j];
			}
			sendBuf[3] = 0x10;
			for (int j = 0; j < 4; j++)
				sendBuf[20 + j] = (char)(sizeof(name) >> (24 - 8 * j));
		}
		else
		{
			for (int j = 0; j < 16356; j++)
				sendBuf[j + 28] = content[index - 1][j];
			sendBuf[3] = 0;
		}
		for (int j = 0; j < 4; j++)
			sendBuf[20 + j] = (char)(16356 >> (24 - 8 * j));
	}
	sendBuf[16] = (char)((want_to_send) >> 8);
	sendBuf[17] = (char)(want_to_send);
	calculatechecksum();
}

void want_to_break()
{
	for (int i = 0; i < 4; i++)
		sendBuf[i] = 0;
	sendBuf[2] = 1;
	for (int i = 0; i < 4; i++)
		sendBuf[i + 16] = (char)(seq >> (24 - 8 * i));
	calculatechecksum();
}

int base = 0;
int has_send = 0;
int calculateack()
{
	return ((int)recvBuf[18] << 8) + (int)recvBuf[19];
}

//窗口内包含发送的是第几个，发送时间是多少
//用队列
//窗口最左边用base=seq初始值，最右边用has_send=seq-1初始值
queue<pair<int, int>> timer_list;
//主线程循环判断是否成功发送所有报文，即base是否为seq+sum+1
//上锁：判断窗口是否满（has_send-base<WINDOW_SIZE）
//并且已经发送的
//没满则继续发送，发送seq为++has_send
//在队列里加入这个seq和发送时间
//
//上锁：判断发送时间，如果超过规定时间，将队列中所有的内容pop
//has_send变成base-1

//接受线程
//判断是否接收到消息，如果接收到消息，判断ACK是否在窗口内
//如果大于等于窗口最小的那个，窗口移动到ack+1
//此时要做的包括将base提到ack+1，队列中的内容pop到相应的ack

HANDLE hMutex = NULL;//互斥量
DWORD WINAPI recvthread(LPVOID lpParamter)
{
	SOCKET sockSrv = (SOCKET)(LPVOID)lpParamter;
	while (1)
	{
		//请求一个互斥量锁
		//cout << "---------------------------------------------------------------" << endl;
		//cout << "              收到报文" << endl;

		if (recvfrom(sockSrv, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&addrServer, &len) != -1
			&& recvBuf[0]
			&& verifychecksum())
		{
		WaitForSingleObject(hMutex, INFINITE);
			int ack = (((unsigned int)((unsigned char)recvBuf[18]) << 8)) + (((unsigned int)(unsigned char)recvBuf[19]));
			//cout << "base" << base << endl;
			//cout << "ack" << ack << endl;
			//printrecv();
			if (ack >= base && ack <= has_send)
			{
				//cout << "timer_list.front().first" << timer_list.front().first << endl;
				while (timer_list.size()!=0&&(timer_list.front().first <= ack))
				{
					//到达最后一个时base=115
					timer_list.pop();
					base++;
					//cout << "base=" << base << endl;
				}
			}
			cout << "左 " << base << "  右  " << has_send << endl;
			ReleaseMutex(hMutex);
		}//释放互斥量锁
		//cout << "收到报文的错误：" << GetLastError() << endl;
	}
	return 0L;//表示返回的是long型的0
}

int main()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(1, 1);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)return 0;
	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
	{
		WSACleanup();
		return 0;
	}
	printf("Client is operating!\n\n");
	SOCKET sockSrv = socket(AF_INET, SOCK_DGRAM, 0);
	addrServer.sin_addr.s_addr = inet_addr(IPServer);
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(PortServer);

	int time_out = 1;//1ms超时
	setsockopt(sockSrv, SOL_SOCKET, SO_RCVTIMEO, (char*)&time_out, sizeof(time_out));

	setIPandPort();
	want_to_connect();
	while (1)
	{
		sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrServer, len);
		cout << "想要连接" << endl;
		if(recvfrom(sockSrv, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&addrServer, &len) != -1)
		{
			if (verifychecksum() && recvBuf[0] == 1)
			{
				cout << "成功连接" << endl;
				seq++;
				break;
			}
		}
	}
	//此时seq=1
	log();
	//所有的文件内容已经装进content数组

	cout <<"seq+sum+1" << seq + sum + 1 << endl;

	base = seq;
	has_send = seq - 1;

	int t_start = clock();

	HANDLE recvThread = CreateThread(NULL, 0, recvthread, (LPVOID)sockSrv, 0, NULL);
	hMutex = CreateMutex(NULL, FALSE, L"screen");
	CloseHandle(recvThread);

	while (1)
	{
		if (recvBuf[2] == 1)
			break;
		//判断是否成功发送所有报文
		WaitForSingleObject(hMutex, INFINITE);
		//cout << "---------------------------------------------------------------" << endl;
		//cout << "               判断是否成功发送所有报文" << endl;
		//cout << "窗口左" << base << endl;
		//cout << "窗口右" << has_send << endl;
		if (base >= seq + sum+1)
		{
			cout << "全部发送" << endl;
			break;
		}
		ReleaseMutex(hMutex);

		//判断窗口是否满（has_send-base<WINDOW_SIZE）
		WaitForSingleObject(hMutex, INFINITE);
		//cout << "---------------------------------------------------------------" << endl;
		//cout << "                  判断窗口大小" << endl;
		//cout << "窗口左" << base << endl;
		//cout << "窗口右" << has_send << endl;
		//cout << "窗口大小" << has_send - base << endl;
		//如果没满，也没有全部发送，就根据has_send继续发送一个
		while (1)
		{
			if (((has_send - base) < WINDOW_SIZE) && has_send < seq + sum + 1)
			{
				//cout << "左 " << base << "  右  " << has_send << endl;
				want_to_sendfile(++has_send);
				sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrServer, len);
				timer_list.push(make_pair(has_send, clock()));
			}
			else break;
		}
		ReleaseMutex(hMutex);

		//cout << "从装填出来了" << endl;

		//判断发送时间
		WaitForSingleObject(hMutex, INFINITE);
		//cout << "---------------------------------------------------------------" << endl;
		//cout << "                  判断发送时间" << endl;
		//cout << "窗口左" << base << endl;
		//cout << "窗口右" << has_send << endl;
		//cout << "窗口大小" << has_send - base << endl;
		if (timer_list.size() != 0)
		{
			//cout << "目前延迟时间" << clock() - timer_list.front().second << endl;
			if ((clock() - timer_list.front().second) > TIMEOUT)
			{
				//cout << "时间到了" << endl;
				has_send = base - 1;
				while (timer_list.size()) timer_list.pop();
			}
		}
		ReleaseMutex(hMutex);
	}
	int t_end = clock();

	WaitForSingleObject(hMutex, INFINITE);
	want_to_break();
	while (1)
	{
		//printsend();
		sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrServer, len);
		cout << "想要断开" << endl;
		if (recvfrom(sockSrv, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&addrServer, &len) != -1)
		{
			//printrecv();
			if (verifychecksum() && recvBuf[0] == 1)
			{
				cout << "成功断开" << endl;
				break;
			}
		}
	}
	cout << "发送" << (sum * 16356 + endfilesize) << "字节" << (t_end - t_start) << "毫秒" << endl;
	cout << "平均吞吐率" << (sum * 16356 + endfilesize) * 8 * 1.0 / (t_end - t_start) * CLOCKS_PER_SEC << " bps" << endl;
	closesocket(sockSrv);
	WSACleanup();return 0;
	ReleaseMutex(hMutex);
}
