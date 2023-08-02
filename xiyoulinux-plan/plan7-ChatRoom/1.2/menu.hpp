#ifndef MENU_HPP
#define MENU_HPP

// 菜单界面，对接的是客户端
// 几个函数，客户端在不同的菜单栏选择不同的选项，进入不同的模块
// 在不同的模块下对用户输入的各种数据进行json序列化和IO的SendMsg
// 并且接收RecvMsg来自服务端发送回来的数据，进行反序列化，打印在页面上
#include "data.h"
#include "define.h"
#include "head.h"
#include "IO.h"
#include "personalmenu.hpp"

#include <iostream>
using json = nlohmann::json;
using namespace std;

// menu调用了下面的函数，因这几个函数是在同一个文件里的，所以得把声明写在menu的前面
// 否者会显示调用失败
void login_client(int client_socket);
void register_client(int client_socket);
void signout_client(int client_socket);

void menu(int client_socket)
{
    while (1)
    {
        cout << "——————————————————————————————————————————————————" << endl;
        cout << "------------------欢迎进入聊天室！-------------------" << endl;
        cout << "——————————————————————————————————————————————————" << endl;
        cout << "---------------------1.登录------------------------" << endl;
        cout << "---------------------2.注册------------------------" << endl;
        cout << "---------------------3.注销------------------------" << endl;
        cout << "——————————————————————————————————————————————————" << endl;

        int num;
        cin >> num;

        switch (num)
        {
        case 1:
            login_client(client_socket); // 传一个客户端的socket进来
            break;
        case 2:
            register_client(client_socket);
            break;
        case 3:
            signout_client(client_socket);
            break;
        default:
            cout << "无效的数字，请重新输入！" << endl;
            break;
        }
    }
}

void login_client(int client_socket)
{
    User user;
    cout << "请输入id: ";
    cin >> user.id;
    cout << "请输入密码: ";
    cin >> user.password;
    user.flag = LOGIN; // 表示是要登录

    // json序列化，及向服务器发送数据
    nlohmann::json sendJson_client = {
        {"id", user.id},
        {"password", user.password},
        {"flag", user.flag},
    };
    string sendJson_client_string = sendJson_client.dump();
    SendMsg sendmsg;
    sendmsg.SendMsg_client(client_socket, sendJson_client_string);

    // 接收数据
    int state_;
    RecvMsg recvmsg;
    state_ = recvmsg.RecvMsg_int(client_socket);

    // 判断是否登入成功
    if (state_ == SUCCESS)
    {
        cout << "登入成功！" << endl;
        //********一个进入下一页面的入口********
        messagemenu(client_socket, user.id);
        system("clear"); // 刷新终端页面
    }
    else if (state_ == FAIL)
    {
        cout << "密码错误！" << endl;
        //*********再次回到登入界面重新输入***********
        return;
    }
    else if (state_ == ONLINE)
    {
        cout << "你已在别处登录！" << endl;
        //*********再次回到登入界面重新输入***********
        return;
    }
    else if (state_ == USERNAMEUNEXIST)
    {
        cout << "该id不存在，请注册 或 重新输入" << endl;
        //*********再次回到登入界面重新输入***********
        return;
    }
    return;
}

void register_client(int client_socket)
{
    User user;
    cout << "请输入用户名: ";
    cin >> user.username;
    cout << "请输入密码: ";
    cin >> user.password;
    user.flag = REGISTER; // 表示是要注册

    // 序列化，发送数据
    nlohmann::json sendJson_client = {
        {"username", user.username},
        {"password", user.password},
        {"flag", user.flag},
    };
    string sendJson_client_string = sendJson_client.dump();
    SendMsg sendmsg;
    sendmsg.SendMsg_client(client_socket, sendJson_client_string);

    // 接收数据
    int state_;
    RecvMsg recvmsg;
    state_ = recvmsg.RecvMsg_int(client_socket);

    // 判断是否注册成功
    if (state_ == SUCCESS)
    {
        string id;
        recvmsg.RecvMsg_client(client_socket, id);
        cout << "注册成功！ 你的账号为：" << id << endl;
        //*******回到登入界面进行登录*********
        return;
    }
    else if (state_ == USERNAMEEXIST)
    {
        cout << "该用户名已存在，请登录 或 更改用户名后重新注册" << endl;
        //*********再次回到登入界面重新注册*************
        return;
    }
    else if (state_ == FAIL)
    {
        cout << "注册失败！" << endl;
        return;
    }
    return;
}

void signout_client(int client_socket)
{
    User user;
    cout << "请输入id: ";
    cin >> user.id;
    cout << "请输入密码: ";
    cin >> user.password;
    user.flag = SIGNOUT; // 表示是要注销

    // 序列化，发送数据（不用把结构体的所有成员都序列化）
    nlohmann::json sendJson_client = {
        {"id", user.id},
        {"password", user.password},
        {"flag", user.flag},
    };
    string sendJson_client_string = sendJson_client.dump();
    SendMsg sendmsg;
    sendmsg.SendMsg_client(client_socket, sendJson_client_string);

    // 接收数据
    int state_;
    RecvMsg recvmsg;
    state_ = recvmsg.RecvMsg_int(client_socket);

    // 判断是否注销成功
    if (state_ == SUCCESS)
    {
        cout << "注销成功！" << endl;
        //*******回到登入界面，看用户是否注册登入*********
        return;
    }
    else if (state_ == USERNAMEUNEXIST)
    {
        cout << "该用户名不存在，请注册 或 重新输入" << endl;
        //*********再次回到登入界面重新注销*************
    }
    else
    {
        cout << "注销失败！---请检查密码输入是否正确" << endl;
        //*********再次回到登入界面重新注销*************
        return;
    }
}

#endif