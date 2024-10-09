#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

SOCKET clientSocket;
SOCKADDR_IN serverAddr;

DWORD WINAPI  ThreadFunction() {
    char receiveMessage[1024];
    int receiveBytes = recv(clientSocket, receiveMessage, sizeof(receiveMessage)-1, 0);
    if (receiveBytes > 0) {
        // 为接收到的数据添加字符串结束符
        receiveMessage[receiveBytes] = '\0';
        cout << "Received message: " << receiveMessage << endl;
    }
    else if (receiveBytes == 0) {
        cout << "Connection closed" << endl;
    }
    else {
        int error = WSAGetLastError();
        cout << "recv failed: " << error << endl;
    }
}
int main() {
    //初始化WinSock
    WSAData wsadata;
    int     res = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (res != 0) {
        cout<<"Winsock initialization failed!" << endl;
        return 1;
    }

    //创建socket
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        cout << "Socket creation failed!" << endl;
        WSACleanup();
        return 1;
    }
    //连接到服务器
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr.S_un.S_addr);


    res = connect(clientSocket,(SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (res != 0) {
        cout << "Connection failed!" << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadFunction, NULL,0,0);
    //发送和接收数据
    char sendMessage[1024] = {};
    while (true) {
        cout << "Enter message: ";
        cin.getline(sendMessage, 1024);
        send(clientSocket, sendMessage, strlen(sendMessage), 0); 
        // 检查是否输入了退出命令
        if (strcmp(sendMessage, "我要退出") == 0) {
            break;
        }
    }
    //关闭socket
    closesocket(clientSocket);
    WSACleanup();
	return 0;
}
