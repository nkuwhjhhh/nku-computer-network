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
#include <io.h>
#define SEND_MAX 0x4000
#define SEND_TOP 0x1C
#pragma comment(lib, "ws2_32.lib")
using namespace std;

char sendBuf[SEND_MAX];
char recvBuf[SEND_MAX];
char content[10000][16356];//传输内容先存在数组里
char name[100];
int sum = 0;//文件报文个数
int index = 0;//设置读入文件的第几个段
int endfilesize = 0;
int seq = 0;

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
	ifstream fin(files[choose].c_str(),ifstream::binary);
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
	sum++;
	cout << "文件报文个数" << sum << endl;
	cout << "最后一个文件报文大小" << endfilesize << endl;
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
	//取反
	calchecksum = ~(u_short)sum;
	sendBuf[26] = (char)(calchecksum >> 8);
	//cout << "sendBuf[26]" << (char)(calchecksum >> 8) << endl;
	sendBuf[27] = (char)calchecksum;
	//cout << "sendBuf[27]" << (char)calchecksum << endl;
}
u_short checksum;
//验证校验和
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
		cout << "成功校验" << endl;
		return true;
	}
	{
		cout << "失败校验" << endl;
		return false;
	}
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

void setseq()
{
	sendBuf[16] = (char)(seq >> 24);
	sendBuf[17] = (char)(seq >> 16);
	sendBuf[18] = (char)(seq >> 8);
	sendBuf[19] = (char)(seq);
}

//将所有flag设置为0
void setflag()
{
	for (int i = 0; i < 4; i++)
	{
		sendBuf[i] = 0;
	}
}

//希望建立连接
void want_to_connect()
{
	setflag();
	//syn设为1其余设置为0
	sendBuf[1] = 1;
	sendBuf[20] = sendBuf[21] = sendBuf[22] = sendBuf[23] = sendBuf[24] = sendBuf[25] = 0;
	setseq();
	calculatechecksum();
}

//对最后一个设置结束标志，同时将结束位置放进去
void want_to_sendfile()
{
	setflag();
	//对每一个都设置index
	sendBuf[24] = (char)(index >> 8);
	sendBuf[25] = (char)index;

	//将filelength所在初始化为0
	for (int i = 20; i < 24; i++) sendBuf[i] = 0;

	//最后一个设置filelength还有多少字节
	if (index == sum)
	{
		sendBuf[20] = (char)(endfilesize >> 24);
		sendBuf[21] = (char)(endfilesize >> 16);
		sendBuf[22] = (char)(endfilesize >> 8);
		sendBuf[23] = (char)endfilesize;
		sendBuf[3] = 0x1;
	}
	else
	{
		if (index == 0)
		{
			sendBuf[3] = 0x10;
			sendBuf[20] = (char)(sizeof(name) >> 24);
			sendBuf[21] = (char)(sizeof(name) >> 16);
			sendBuf[22] = (char)(sizeof(name) >> 8);
			sendBuf[23] = (char)sizeof(name);
		}
		else
		{
			sendBuf[3] = 0;
		}
		sendBuf[20] = (char)(16356 << 24);
		sendBuf[21] = (char)(16356 << 16);
		sendBuf[22] = (char)(16356 << 8);
		sendBuf[23] = (char)16356;
	}
	setseq();
	calculatechecksum();
}

void want_to_break()
{
	setflag();
	sendBuf[2] = 1;
	setseq();
	calculatechecksum();
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
	if(a==0)
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
		<< "      seq "<<(int)seq
		<< "      ACK " << (int)sendBuf[0]
		<< "      SYN " << (int)sendBuf[1]
		<< "      FIN " << (int)sendBuf[2]
		<< "      校验和 " << (int)calchecksum
		<< "      index " << (int)index;
	if (index == sum)
	{
		cout << "       length " << endfilesize << endl;
	}
	else
	{
		cout << "       length " << 16356 << endl;
	}
}



void main()
{
	//加载套接字库
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(1, 1);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		return;
	}

	if (LOBYTE(wsaData.wVersion) != 1 ||     //低字节为主版本
		HIBYTE(wsaData.wVersion) != 1)      //高字节为副版本
	{
		WSACleanup();
		return;
	}

	printf("Client is operating!\n\n");
	//创建用于监听的socket
	SOCKET sockSrv = socket(AF_INET, SOCK_DGRAM, 0);


	SOCKADDR_IN  addrServer;
	addrServer.sin_addr.s_addr = inet_addr(IPServer);//输入你想通信的她（此处是本机内部）
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(PortServer);

	int len = sizeof(SOCKADDR);

	//将IP，port，seq=0，SYN=1，ACK=0

	setIPandPort();
	want_to_connect();
	while (1)
	{
		//发送第一个报文连接，计算校验和
		getlog1(1);
		sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrServer, len);
		cout << "要接受连接回应了" << endl;
		recvfrom(sockSrv, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&addrServer, &len);

		//接收第一个报文连接，判断ack是否等于1，输出连接成功并且break
		//如果不等于1，continue，继续发报文连接
		if (verifychecksum()&&recvBuf[0] == 1)
		{
			seq++;
			getlog1(0);
			break;
		}
		getlog1(0);
		cout << "没连上再来一次" << endl;
	}

	log();//选择发送哪一个文件
		  //计算文件总长度，计算sum

	int t_start = clock();
	while (index < sum+1)
	{
		getlog();
		//开始发送文件
		want_to_sendfile();
		if (index == 0)
		{
			for (int j = 0; j < sizeof(name); j++)
			{
				sendBuf[j + 28] = name[j];
			}
		}
		else 
		{
			if (index == sum)
			{
				for (int j = 0; j < endfilesize; j++)
					sendBuf[j + 28] = content[index - 1][j];
			}
			else
			{
				for (int j = 0; j < 16356; j++)
					sendBuf[j + 28] = content[index - 1][j];
			}
		}
		while (1)
		{
			sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrServer, len);

			//等待并数据
			recvfrom(sockSrv, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&addrServer, &len);
			getlog1(0);
			//如果ACK为1，继续发送下一个
			if (verifychecksum()&&recvBuf[0] == 1) break;
		}
		seq++;
		index++;
	}
	int t_end = clock();
	//发送断开连接报文，计算校验和并发送出去
	want_to_break();
	while (1)
	{
		index = 0;
		cout << "发送了3" << endl;
		getlog1(1);
		sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrServer, len);

		//接收最后一个报文断开，判断fin是否等于1，输出断开成功并且break
		recvfrom(sockSrv, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&addrServer, &len);
		getlog1(0);
		//如果不等于1，continue，继续发报文连接
		if (verifychecksum()&&recvBuf[0] == 1) break;
	}
	cout << "发送" << (sum * 16356 + endfilesize) << "字节" << (t_end - t_start) << "毫秒" << endl;
	cout << "平均吞吐率" << (sum * 16356 + endfilesize) * 8 * 1.0 / (t_end - t_start) * CLOCKS_PER_SEC << " bps" << endl;
	closesocket(sockSrv);
	WSACleanup();
}


//只跑程序


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
#include <io.h>
#define SEND_MAX 0x4000
#define SEND_TOP 0x1C
#pragma comment(lib, "ws2_32.lib")
using namespace std;

char sendBuf[SEND_MAX];
char recvBuf[SEND_MAX];
char content[10000][16356];//传输内容先存在数组里
char name[100];
int sum = 0;//文件报文个数
int index = 0;//设置读入文件的第几个段
int endfilesize = 0;
int seq = 0;

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
	sum++;
	cout << "文件报文个数" << sum << endl;
	cout << "最后一个文件报文大小" << endfilesize << endl;
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
	//取反
	calchecksum = ~(u_short)sum;
	sendBuf[26] = (char)(calchecksum >> 8);
	//cout << "sendBuf[26]" << (char)(calchecksum >> 8) << endl;
	sendBuf[27] = (char)calchecksum;
	//cout << "sendBuf[27]" << (char)calchecksum << endl;
}
u_short checksum;
//验证校验和
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

void setseq()
{
	sendBuf[16] = (char)(seq >> 24);
	sendBuf[17] = (char)(seq >> 16);
	sendBuf[18] = (char)(seq >> 8);
	sendBuf[19] = (char)(seq);
}

//将所有flag设置为0
void setflag()
{
	for (int i = 0; i < 4; i++)
	{
		sendBuf[i] = 0;
	}
}

//希望建立连接
void want_to_connect()
{
	setflag();
	//syn设为1其余设置为0
	sendBuf[1] = 1;
	sendBuf[20] = sendBuf[21] = sendBuf[22] = sendBuf[23] = sendBuf[24] = sendBuf[25] = 0;
	setseq();
	calculatechecksum();
}

//对最后一个设置结束标志，同时将结束位置放进去
void want_to_sendfile()
{
	setflag();
	//对每一个都设置index
	sendBuf[24] = (char)(index >> 8);
	sendBuf[25] = (char)index;

	//将filelength所在初始化为0
	for (int i = 20; i < 24; i++) sendBuf[i] = 0;

	//最后一个设置filelength还有多少字节
	if (index == sum)
	{
		sendBuf[20] = (char)(endfilesize >> 24);
		sendBuf[21] = (char)(endfilesize >> 16);
		sendBuf[22] = (char)(endfilesize >> 8);
		sendBuf[23] = (char)endfilesize;
		sendBuf[3] = 0x1;
	}
	else
	{
		if (index == 0)
		{
			sendBuf[3] = 0x10;
			sendBuf[20] = (char)(sizeof(name) >> 24);
			sendBuf[21] = (char)(sizeof(name) >> 16);
			sendBuf[22] = (char)(sizeof(name) >> 8);
			sendBuf[23] = (char)sizeof(name);
		}
		else
		{
			sendBuf[3] = 0;
		}
		sendBuf[20] = (char)(16356 << 24);
		sendBuf[21] = (char)(16356 << 16);
		sendBuf[22] = (char)(16356 << 8);
		sendBuf[23] = (char)16356;
	}
	setseq();
	calculatechecksum();
}

void want_to_break()
{
	setflag();
	sendBuf[2] = 1;
	setseq();
	calculatechecksum();
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
		cout << "       length " << endfilesize << endl;
	}
	else
	{
		cout << "       length " << 16356 << endl;
	}
}



void main()
{
	//加载套接字库
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(1, 1);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		return;
	}

	if (LOBYTE(wsaData.wVersion) != 1 ||     //低字节为主版本
		HIBYTE(wsaData.wVersion) != 1)      //高字节为副版本
	{
		WSACleanup();
		return;
	}

	printf("Client is operating!\n\n");
	//创建用于监听的socket
	SOCKET sockSrv = socket(AF_INET, SOCK_DGRAM, 0);


	SOCKADDR_IN  addrServer;
	addrServer.sin_addr.s_addr = inet_addr(IPServer);//输入你想通信的她（此处是本机内部）
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(PortServer);

	int len = sizeof(SOCKADDR);

	//将IP，port，seq=0，SYN=1，ACK=0

	setIPandPort();
	want_to_connect();
	while (1)
	{
		//发送第一个报文连接，计算校验和
		//getlog1(1);
		sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrServer, len);
		//cout << "要接受连接回应了" << endl;
		recvfrom(sockSrv, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&addrServer, &len);

		//接收第一个报文连接，判断ack是否等于1，输出连接成功并且break
		//如果不等于1，continue，继续发报文连接
		if (verifychecksum() && recvBuf[0] == 1)
		{
			seq++;
			//getlog1(0);
			break;
		}
		//getlog1(0);
		//cout << "没连上再来一次" << endl;
	}

	log();//选择发送哪一个文件
		  //计算文件总长度，计算sum

	int t_start = clock();
	while (index < sum + 1)
	{
		//getlog();
		//开始发送文件
		want_to_sendfile();
		if (index == 0)
		{
			for (int j = 0; j < sizeof(name); j++)
			{
				sendBuf[j + 28] = name[j];
			}
		}
		else
		{
			if (index == sum)
			{
				for (int j = 0; j < endfilesize; j++)
					sendBuf[j + 28] = content[index - 1][j];
			}
			else
			{
				for (int j = 0; j < 16356; j++)
					sendBuf[j + 28] = content[index - 1][j];
			}
		}
		while (1)
		{
			sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrServer, len);

			//等待并数据
			recvfrom(sockSrv, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&addrServer, &len);
			//getlog1(0);
			//如果ACK为1，继续发送下一个
			if (verifychecksum() && recvBuf[0] == 1) break;
		}
		seq++;
		index++;
	}
	int t_end = clock();
	//发送断开连接报文，计算校验和并发送出去
	want_to_break();
	while (1)
	{
		index = 0;
		//cout << "发送了3" << endl;
		//getlog1(1);
		sendto(sockSrv, sendBuf, sizeof(sendBuf), 0, (SOCKADDR*)&addrServer, len);

		//接收最后一个报文断开，判断fin是否等于1，输出断开成功并且break
		recvfrom(sockSrv, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&addrServer, &len);
		//getlog1(0);
		//如果不等于1，continue，继续发报文连接
		if (verifychecksum() && recvBuf[0] == 1) break;
	}
	cout << "发送" << (sum * 16356 + endfilesize) << "字节" << (t_end - t_start) << "毫秒" << endl;
	cout << "平均吞吐率" << (sum * 16356 + endfilesize) * 8 * 1.0 / (t_end - t_start) * CLOCKS_PER_SEC << " bps" << endl;
	closesocket(sockSrv);
	WSACleanup();
}