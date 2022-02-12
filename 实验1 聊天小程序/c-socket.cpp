//客户端
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#include<iostream>
#include <winsock2.h>
#include<time.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

int main()
{
    //第一步：初始化
    WSAData wsaData;
    WORD wVersionRequested;
    wVersionRequested = MAKEWORD(2, 2);
    WSAStartup(wVersionRequested, &wsaData);

    //第二步：创建一个socket
    SOCKET sockClient = {};
    sockClient = socket(PF_INET, SOCK_STREAM, 0);
    if (sockClient == SOCKET_ERROR)cout << "socket创建失败" << endl;
    else cout << "socket创建成功" << endl;

    //第三步：向一个特定的socket发出建连请求
    sockaddr_in addrSrv;
    addrSrv.sin_family = PF_INET;
    addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    addrSrv.sin_port = htons(1234);
    int cRes = connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
    if (SOCKET_ERROR == cRes)cout << "连接失败" << endl;
    else cout << "连接成功" << endl;

    cout << "2号" << endl;
    cout << "————————————————————————" << endl;

    while (1)
    {
        //发送
        char sendBuf[1024] = {};
        cout << "2号：";
        cin.getline(sendBuf, 1024);

        //判断自己是否要退出
        if (!strcmp("quit", sendBuf))
        {
            cout << "要退出了" << endl;
            break;
        }

        //给sendBuf加一个发送时间
        time_t now_time1 = time(NULL);
        tm* t_tm1 = localtime(&now_time1);
        cout << "                自己发送时间：" << asctime(t_tm1);
        strcat(sendBuf, "        对方发送时间：");
        strcat(sendBuf, asctime(t_tm1));
        send(sockClient, sendBuf, strlen(sendBuf), 0);

        cout << "—————————————————————————————" << endl;

        //接收，给recvBuf加一个收到时间
        char recvBuf[1024] = {};
        recv(sockClient, recvBuf, 1024, 0);
        cout << "1号：" << recvBuf;
        time_t now_time = time(NULL);
        tm* t_tm = localtime(&now_time);
        cout << "                自己收到时间：" << asctime(t_tm);

        //判断是否对方要退出
        if (strlen(recvBuf) == 0)
        {
            cout << "要退出了" << endl;
            break;
        }

        cout << "—————————————————————————————" << endl;
    }

    //关闭socket
    closesocket(sockClient);
    //结束使用socket，释放socket dll资源
    WSACleanup();
}