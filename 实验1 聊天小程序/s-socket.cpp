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
    WSAData wsaData; //实例化wsaData
    WORD wVersionRequested;
    wVersionRequested = MAKEWORD(2, 2);//指定版本wVersionRequested=MAKEWORD(a,b)
    WSAStartup(wVersionRequested, &wsaData);

    //第二步：建立一个socket，并绑定到一个特定的传输层服务
    SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);
    if (!sockSrv)cout << "建立socket失败" << endl;
    else cout << "建立socket成功" << endl;

    //第三步：bind，将一个本地地址绑定到指定的Socket
    SOCKADDR_IN addrSrv;
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    addrSrv.sin_port = htons(1234);
    int cRes = bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
    if (SOCKET_ERROR == cRes)   cout << "绑定失败" << endl;
    else cout << "绑定成功" << endl;

    //第四步：listen，使socket进入监听状态，监听远程连接是否到来
    if (listen(sockSrv, 5))cout << "监听失败" << endl;
    else cout << "监听成功" << endl;

    //初始化一个客户端，等待客户端的连接
    SOCKADDR_IN addrClient;
    int len = sizeof(sockaddr_in);
    SOCKET sockConn = accept(sockSrv, (SOCKADDR*)&addrClient, &len);
    if (!sockConn)cout << "连接失败" << endl;
    else cout << "连接成功" << endl;

    //成功后
    cout << "1号" << endl;
    cout << "————————————————————————" << endl;
    while (1)
    {
        //接收，给recvBuf加一个收到时间
        char recvBuf[1024] = {};
        recv(sockConn, recvBuf, 1024, 0);
        cout << "2号：" << recvBuf;
        time_t now_time = time(NULL);
        tm* t_tm = localtime(&now_time);
        cout << "                自己收到时间: " << asctime(t_tm);

        //判断是否对方要退出
        if (strlen(recvBuf) == 0)
        {
            cout << "要退出了" << endl;
            break;
        }

        cout << "—————————————————————————————" << endl;

        //发送
        char sendBuf[1024] = {};
        cout << "1号：";
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
        send(sockConn, sendBuf, strlen(sendBuf), 0);

        cout << "—————————————————————————————" << endl;
    }
    closesocket(sockConn);
    closesocket(sockSrv);
    WSACleanup();
}