#include <json.hpp>
#include <iostream>
#include <thread>
#include <string>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "client_help.hpp"

// 聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // 填写client需要连接的server信息ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client和server进行连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    // 初始化读写线程通信用的信号量
    sem_init(&rwsem, 0, 0);

    // 连接服务器成功，启动接收子线程
    std::thread readTask(readTaskHandler, clientfd); // pthread_create
    readTask.detach();                               // pthread_detach

    // main线程用于接收用户输入，负责发送数据
    for (;;)
    {
        // 显示首页面菜单 登录、注册、退出
        cout << "========================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "========================" << endl;
        cout << "choice:";
        int choice = 0;
        while (true) {
            std::cout << "Enter an integer ID: ";

            if (std::cin >> choice) {
                // 输入成功，继续处理
                break;  // 跳出循环
            } else {
                // 输入失败，发生异常
                std::cin.clear();  // 清除错误状态标志
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // 忽略输入缓冲区中的无效字符
                std::cerr << "Invalid input. Please enter an choice: " << std::endl;
            }
        }
        cin.get(); // 读掉缓冲区残留的回车

        switch (choice)
        {
            case 1: // login业务
            {
                int id = 0;
                char pwd[50] = {0};
                cout << "userid:";
                while (true) {

                    if (std::cin >> id) {
                        // 输入成功，继续处理
                        break;  // 跳出循环
                    } else {
                        // 输入失败，发生异常
                        std::cin.clear();  // 清除错误状态标志
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // 忽略输入缓冲区中的无效字符
                        std::cerr << "Invalid input. Please enter an userid." << std::endl;
                    }
                }
                cin.get(); // 读掉缓冲区残留的回车
                cout << "userpassword:";
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = pwd;
                string request = js.dump();

                g_isLoginSuccess = false;

                // 发送数据给服务端端
                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if (len == -1)
                {
                    cerr << "send login msg error:" << request << endl;
                }

                sem_wait(&rwsem); // 等待信号量，由子线程处理完登录的响应消息后，通知这里

                if (g_isLoginSuccess)
                {
                    // 进入聊天主菜单页面
                    isMainMenuRunning = true;
                    mainMenu(clientfd);
                }
            }
                break;
            case 2: // register业务
            {
                char name[50] = {0};
                char pwd[50] = {0};
                cout << "username:";
                cin.getline(name, 50);
                cout << "userpassword:";
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = pwd;
                string request = js.dump();

                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if (len == -1)
                {
                    cerr << "send reg msg error:" << request << endl;
                }

                sem_wait(&rwsem); // 等待信号量，子线程处理完注册消息会通知
            }
                break;
            case 3: // quit业务
                close(clientfd);
                sem_destroy(&rwsem);
                exit(0);
            default:
                cerr << "invalid input!" << endl;
                break;
        }
    }

    return 0;
}

