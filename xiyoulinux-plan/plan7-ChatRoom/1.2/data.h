// 各种结构体及容器
#ifndef DATA_H
#define DATA_H

#include <vector>

#include <iostream>
using namespace std;

// int epld;

// string chatid = "";
string chatname = "";
string chatgroup = "";

// 注册登录（哈希存，键：userinfo，字段：用户id，值：User的json字符串）
struct User
{
    int flag;        // 标记选项
    string username; // 用户名（独一无二）
    string password; // 密码
    string id;       // 服务器随机生成的id（独一无二）--->使用户更改用户名的时候，该用户的数据不变
    int status;      // 标记从服务器返回的状态（成功SUCCESS，失败FAIL）
    int online;      // 是否在线（在线ONLINE，不在OFFLINE）
};

// 好友
struct Friend
{
    int type;        // 标记事件类型（正常NORMAL，通知NOTICE）
    int flag;        // 标记选项
    string username; // 用户名
    string id;       // id
    string opponame; // 对方用户名
    string oppoid;   // 对方id
    string msg;      // 通知消息
    int state;       // 标记从服务器返回的状态（成功SUCCESS，失败FAIL）
    int online;      // 是否在线（在线ONLINE，不在OFFLINE）
};

// 聊天消息(群聊也ok)
struct Chatinfo
{
    string name;
    string msg;
    // string time;
};

// 群聊
struct Group
{
    int type;               // 标记事件类型
    int flag;               // 标记选项
    string groupid;         // 群id
    string groupname;       // 群名
    string ownerid;         // 群主id
    vector<string> adminid; // 群管理们的id
    vector<string> nums;    // 群成员们的id（包括群主和管理员）
    string userid;          // 个人id（加群时用）
    string oppoid;          // 目标成员id（增删管理、踢人）
    string opponame;        // 目标成员名字
    string msg;             // 通知消息
    int state;              // 标记状态
};

// 根据时间分配id的函数
string produce_id(void)
{
    time_t timestamp;
    time(&timestamp);
    string id = to_string(timestamp);
    return id;
}

// 屏蔽ctrl的输入函数
string getInputWithoutCtrlD()
{
    struct termios oldt, newt;

    // 获取当前终端模式
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    // 禁用 ECHO 和 ICANON 标志
    newt.c_lflag &= ~(ICANON | ECHO);

    // 设置新终端模式
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::string input;
    char ch;
    while (std::cin.get(ch))
    {
        if (ch == '\x04')
        { // Ctrl+D
            // 不做任何响应
        }
        else if (ch == '\n')
        {
            std::cout << ch;
            break; // 回车表示输入结束
        }
        else
        {
            std::cout << ch; // 输出字符到终端
            input += ch;
        }
    }

    // 恢复原始终端模式
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    return input;
}

// 检测是否为非法输入的函数
int checkcin(const string &str)
{
    try
    {
        size_t pos;
        int result = std::stoi(str, &pos);

        // 确保整个字符串都被转换
        if (pos == str.length())
        {
            return result;
        }
        else
        {
            return -1;
        }
    }
    catch (const std::invalid_argument &)
    {
        return -1; // 无法转换成数字
    }
    catch (const std::out_of_range &)
    {
        return -1; // 数字超出范围
    }
}

// 根据send和recv函数返回值判断客户端是否正常 来对socket、redis描述符、用户在线状态进行更改
// 函数部分：
void errorprocess(int fd)
{
}

#endif