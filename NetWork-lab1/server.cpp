#include <iostream>
#include <string>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <thread>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

const int maxNum = 4;

SOCKET serverSocket;
SOCKET clientSocket[maxNum];
SOCKADDR_IN serverAddr;
SOCKADDR_IN client_Addr[maxNum];
//表示当前已经使用的客户端数量
int clientNum;
//标识当前的客户端是否被占用
int clientState[maxNum] = { 0 };

DWORD WINAPI ExitFunction() {
    char message[1024] = {};
    while (1) {
        cin.getline(message, sizeof(message));
        if (strcmp(message, "关闭服务器") == 0) {
            exit(0);
        }
        else {
            cout << "指令错误！" << endl;
        }
    }
}

int WaitClient() {
    SOCKADDR_IN clientAddr;
    int addrlen = sizeof(clientAddr);
    // 接受客户端连接请求
    SOCKET newSocket = accept(serverSocket, (SOCKADDR*)&clientAddr, &addrlen);
    if (newSocket == INVALID_SET_FILE_POINTER) {
        cerr << "Accept failed: " << WSAGetLastError() << endl;
        return -1;
    }

    int mark = -1;
    for (int i = 0; i < maxNum; i++) {
        if (clientState[i] == 0) {
            clientSocket[i] = newSocket;
            client_Addr[i] = clientAddr;
            clientState[i] = 1;
            clientNum++;
            mark = i;
            break;
        }
    }
    if (mark == -1) {
        cout << "服务器已满" << endl; 
        closesocket(newSocket); 
        return -1; 
    }


    time_t nowtime = time(nullptr);
    tm* p = localtime(&nowtime);
    cout << "Client" << mark << "已连接! 时间是";
    printf("%04d:%02d:%02d %02d:%02d:%02d\n", p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    cout << "当前在线人数" << clientNum << endl;
    //单独创建消息处理进程，以便并发执行
    thread Thread(ThreadFunction, mark);
    return 0;
}

DWORD WINAPI ThreadFunction(LPVOID lpParameter) {
    // 转换lpParameter为整数，表示客户端索引
    int s = static_cast<int>(reinterpret_cast<intptr_t>(lpParameter));

    char message[1024] = { 0 };
    char recvMessage[1024] = { 0 };

    SOCKET *nowSocket = &clientSocket[s];
    SOCKADDR_IN nowAddr = client_Addr[s];

    while (true) {
        int recvBytes = recv(*nowSocket, recvMessage, sizeof(recvMessage) - 1, 0);
        if (recvBytes > 0) {
            message[recvBytes] = '\0';
            string strMessage(message);

            if (strMessage != "退出") {
                time_t nowtime = time(nullptr);
                tm p;
                localtime_s(&p, &nowtime);

                cout << "Client" << s << " say: " << message << " --";
                printf("%04d:%02d:%02d %02d:%02d:%02d\n", p.tm_year + 1900, p.tm_mon + 1, p.tm_mday, p.tm_hour, p.tm_min, p.tm_sec);

                sprintf_s(recvMessage, sizeof(recvMessage), ">>>>>>>>>>>>>>>>>>>>   Client %d  -- %04d:%02d:%02d %02d:%02d:%02d --   <<<<<<<<<<<<<<<<<<<<\n%s\n>>>>>>>>>>>>>>>>>>>> >>>>>>>>>>>>>>>>   Over   <<<<<<<<<<<<<<<< <<<<<<<<<<<<<<<<<<<<\n",
                    s, p.tm_year + 1900, p.tm_mon + 1, p.tm_mday, p.tm_hour, p.tm_min, p.tm_sec, message);

                for (int i = 0; i < maxNum; i++) {
                    if (clientSocket[i] == 1) {
                        send(clientSocket[i], recvMessage, sizeof(recvMessage), 0);
                    }
                }
            }else {
                time_t nowtime = time(nullptr);
                tm p;
                localtime_s(&p, &nowtime);

                // 打印客户端退出信息
                cout << "Client" << s << " 退出了" << " --";
                printf("%04d:%02d:%02d %02d:%02d:%02d\n", p.tm_year + 1900, p.tm_mon + 1, p.tm_mday, p.tm_hour, p.tm_min, p.tm_sec);

                // 格式化客户端退出的广播消息
                sprintf_s(recvMessage, sizeof(recvMessage), ">>>>>>>>>>>>>>>>>>>> >>>>>>>>>>>>>>>>   退出   <<<<<<<<<<<<<<<< <<<<<<<<<<<<<<<<<<<<\nClient %d 已退出  -- %04d:%02d:%02d %02d:%02d:%02d -- \n>>>>>>>>>>>>>>>>>>>> >>>>>>>>>>>>>>>>   Over   <<<<<<<<<<<<<<<< <<<<<<<<<<<<<<<<<<<<\n",
                    s, p.tm_year + 1900, p.tm_mon + 1, p.tm_mday, p.tm_hour, p.tm_min, p.tm_sec);

                for (int i = 0; i < maxNum; i++) {
                    if (clientState[i] == 1) {
                        send(clientSocket[i], recvMessage, strlen(recvMessage), 0);
                    }
                }
                clientState[s] = 0;
                clientNum--;

                cout << "当前在线人数" << clientNum << endl;
                closesocket(*nowSocket);
                break;
            }
        }
        else {
            if (WSAGetLastError() == 10054) {
                cout << "Client" << s << " 已断开" << endl;
            }
            else {
                // 如果接收消息时出现其他错误
                cout << "接收消息错误: " << WSAGetLastError() << endl;
            }
            closesocket(*nowSocket);
            break;
        
        }
    }
    return 0;
}
int main() {
	//初始化 Winsock
	WSAData wsadata;
	int     res = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (res != 0) {
        std::cout << "Winsock initialization failed!" << std::endl;
        return 1;
    }
	//创建套接字
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cout << "Socket creation failed!" << std::endl;
        WSACleanup();
        return 1;
    }
	//绑定套接字到地址和端口
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr.S_un.S_addr);

    res=bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (res != 0) {
        cout << "Bind failed!" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
	//监听连接请求
    res = listen(serverSocket, 5);
    if (res != 0) {
        cout << "Listen failed!" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Server is listening on port " << 12345 << endl;


    HANDLE Exit_Thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ExitFunction, NULL, 0, NULL);
    // 等待退出线程完成，这里使用INFINITE参数表示无限期等待，直到线程结束
    // 但实际上ExitFunction是一个无限循环，除非程序被外部中断，否则这个线程不会自行结束
    WaitForSingleObject(Exit_Thread, INFINITE);
    // 关闭退出线程的句柄，释放资源
    CloseHandle(Exit_Thread);
   
    while (true) {
        WaitClient();
    }
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}





