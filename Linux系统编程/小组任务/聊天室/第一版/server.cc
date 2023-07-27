// 对客户端发来的各种json进行处理，并将事件加到工作队列，从线程池中取出工作线程进行相应的处理
#include "threadpool.h"
#include "IO.h"
#include "data.h"
#include "define.h"
#include "login.hpp"

#include <iostream>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

using json = nlohmann::json;
#define PORT 8080
const int MAX_CONN = 1024; // 最大连接数

void work(void *arg);

int main()
{
    // 创建线程池(24个线程)
    ThreadPool threadpool(24);

    // 创建监听的socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket error");
        return -1;
    }

std::cout <<  "1" << std::endl;
    // 绑定本地ip和端口
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    int ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if (sockfd < 0)
    {
        perror("bind error");
        return -1;
    }
std::cout <<  "2" << std::endl;
    // 监听客户端连接
    ret = listen(sockfd, 1024);
    if (ret < 0)
    {
        perror("listen error");
        return -1;
    }

    // 创建epoll实例
    int epld = epoll_create(10);
    if (epld < 0)
    {
        perror("epoll_create error");
        return -1;
    }
std::cout <<  "3" << std::endl;
    // 将监听的socket加入epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    ret = epoll_ctl(epld, EPOLL_CTL_ADD, sockfd, &ev);
    if (ret < 0)
    {
        perror("epoll_ctl error");
        return -1;
    }
std::cout <<  "4" << std::endl;

    // 循环监听
    while (1)
    {
        struct epoll_event evs[MAX_CONN];
        int n = epoll_wait(epld, evs, MAX_CONN, -1);
        if (n < 0) 
        {
            perror("epoll_wait error");
            break;
        }
std::cout <<  "5" << std::endl;
        for (int i = 0; i < n; i++)
        {
            int fd = evs[i].data.fd;
            
            // 如果是监听的fd收到消息，那么则表示有客户端进行连接了
            if (fd == sockfd)
            {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
                if (client_sockfd < 0)
                {
                    perror("accept error");
                    continue;
                }
std::cout <<  client_sockfd << std::endl;
                // 将客户端的socket加入epoll
                struct epoll_event ev_client;
                ev_client.events = EPOLLIN; // 检测客户端有没有消息过来
                ev_client.data.fd = client_sockfd;

                // // 设置非阻塞
                int flag;
                flag = fcntl(client_sockfd, F_GETFL);
                flag |= O_NONBLOCK;
                fcntl(client_sockfd, F_SETFL, flag); // 设置非阻塞

                // 心跳检测（开启保活，1分钟内探测不到，断开连接）
                int keep_alive = 1;
                int keep_idle = 3;
                int keep_interval = 1;
                int keep_count = 30;
                if (setsockopt(client_sockfd, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(keep_alive)))
                {
                    perror("Error setsockopt(SO_KEEPALIVE) failed");
                    exit(1);
                }
                if (setsockopt(client_sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle, sizeof(keep_idle)))
                {
                    perror("Error setsockopt(TCP_KEEPIDLE) failed");
                    exit(1);
                }
                if (setsockopt(client_sockfd, SOL_TCP, TCP_KEEPINTVL, (void *)&keep_interval, sizeof(keep_interval)))
                {
                    perror("Error setsockopt(TCP_KEEPINTVL) failed");
                    exit(1);
                }
                if (setsockopt(client_sockfd, SOL_TCP, TCP_KEEPCNT, (void *)&keep_count, sizeof(keep_count)))
                {
                    perror("Error setsockopt(TCP_KEEPCNT) failed");
                    exit(1);
                }

                // 将该客户端加入epoll树
                ret = epoll_ctl(epld, EPOLL_CTL_ADD, client_sockfd, &ev_client);
                if (ret < 0)
                {
                    perror("client epoll_ctl error");
                    break;
                }
                std::cout << client_addr.sin_addr.s_addr << "正在连接..." << std::endl;
            }

            else // 如果客户端有消息
            {
                // 从树上去除该套接字
                struct epoll_event temp;
                temp.data.fd = fd;
                temp.events = EPOLLIN;
                epoll_ctl(epld, EPOLL_CTL_DEL, fd, &temp);

                // 进入子线程，接下来的任务由子线程完成
                int arg[] = {fd, epld}; // 把fd和efd装到arg数组里，再作为参数传到work函数里，arg[0]=fd,arg[1]=epld
                Task task;
                task.function = work;
                task.arg = arg;
                threadpool.addTask(task);
            }
        }
    }

    // 关闭epoll实例及套接字
    close(epld);
    close(sockfd);
}

void work(void *arg)
{
    // 挂树的准备工作
    struct epoll_event temp;
    int *arr = (int *)arg;
    int fd = arr[0];
    int epld = arr[1];
    temp.data.fd = fd;
    temp.events = EPOLLIN; // 以上代码为后面 挂树 时有用，也是该函数传入参数的唯一作用

    // 服务端开的子线程负责接收客户端的消息
    string recvJson_buf;
    RecvMsg recvmsg;
    recvmsg.RecvMsg_client(fd, recvJson_buf);//***********第二次回到登录界面进行选择时，按下回车键，服务器会出现段错误，且停在这行过不去
    json parsed_data = json::parse(recvJson_buf);
    User un_user;
    un_user.username = parsed_data["username"];
    un_user.password = parsed_data["password"];
    un_user.flag = parsed_data["flag"];

    // 根据接收到的消息判断用户在登录界面的操作
    if (un_user.flag == LOGIN) // 登录
    {
        login_server(fd,un_user,recvJson_buf);
    }
    else if (un_user.flag == REGISTER) // 注册
    {
        register_server(fd,un_user,recvJson_buf);
    }
    else if (un_user.flag == SIGNOUT) // 注销
    {
        signout_server(fd,un_user);
    }

    // 当所有任务都处理完了（或出问题）之后，再挂树
    epoll_ctl(epld, EPOLL_CTL_ADD, fd, &temp);
    return;
}