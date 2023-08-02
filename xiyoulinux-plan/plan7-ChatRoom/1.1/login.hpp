// 服务端处理客户端在登录界面输入的注册、登入、注销
#include "data.h"
#include "define.h"
#include "redis.hpp"
#include "IO.h"

#include <iostream>
using json = nlohmann::json;
using namespace std;

// 登录
void login_server(int fd, string buf)
{
    json parsed_data = json::parse(buf);
    struct User user;
    user.id = parsed_data["id"];
    user.password = parsed_data["password"];
    printf("--- %s 用户将要登录 ---\n", user.id.c_str());

    // 下面两种都行，但第一种的话要加上.c_str()
    printf("id：%s\n密码：%s\n", user.id.c_str(), user.password.c_str());
    // cout << user.username << '\n' << user.password << endl;

    Redis redis;
    redis.connect();

    if (redis.hashexists("userinfo", user.id) != 1) // 账号不存在
    {
        cout << "该id不存在，请注册 或 重新输入" << endl;
        SendMsg sendmsg;
        sendmsg.SendMsg_int(fd, USERNAMEUNEXIST);
    }
    else // 账号存在
    {
        string userjson_string;
        userjson_string = redis.gethash("userinfo", user.id);
        parsed_data = json::parse(userjson_string);
        if (user.password != parsed_data["password"])
        {
            cout << "密码错误" << endl;
            SendMsg sendmsg;
            sendmsg.SendMsg_int(fd, FAIL);
        }
        else if (redis.sismember("onlinelist", user.id) == 1)
        {
            cout << "用户已登录" << endl;
            SendMsg sendmsg;
            sendmsg.SendMsg_int(fd, ONLINE);
        }
        else
        {
            cout << "登录成功" << endl;
            SendMsg sendmsg;
            sendmsg.SendMsg_int(fd, SUCCESS);

            // 改变用户的在线状态（将原来的string只改变在线状态后，完整地发送给数据库）
            // int state_ = parsed_data["online"];
            // state_ = ONLINE;
            parsed_data["online"] = ONLINE;
            userjson_string = parsed_data.dump();
            redis.hsetValue("userinfo", user.id, userjson_string);
            redis.saddvalue("onlinelist", user.id); // 在线列表

            //------------登录成功后，是客户端进入下一步，服务端只需要根据客户端发来的请求 调用相应的函数 来处理即可-------------
            // 现在这个登录的任务服务器处理完毕了，也就可以返回上一级了
            // 该线程也就被回收了^_^
        }
    }

    return;
}

// 注册
void register_server(int fd, string buf)
{
    json parsed_data = json::parse(buf);
    struct User user;
    user.username = parsed_data["username"];
    user.password = parsed_data["password"];
    printf("--- %s 用户将要注册 ---\n", user.username.c_str());
    printf("用户名：%s\n密码：%s\n", user.username.c_str(), user.password.c_str());

    Redis redis;
    redis.connect();

    if (redis.sismember("username", user.username) == 1) // 用户名已被使用
    {
        cout << "该用户名已存在，请登录 或 更改用户名后重新注册" << endl;
        SendMsg sendmsg;
        sendmsg.SendMsg_int(fd, USERNAMEEXIST);
    }
    else // 新的用户名，可以被使用
    {
        // 随机生成id
        time_t timestamp;
        time(&timestamp);
        string id = to_string(timestamp);

        // // 初始化好友列表
        // vector<string> emptyvector;
        // nlohmann::json tojson = {
        //     {"friends", emptyvector},
        // };
        // string str = tojson.dump();
        // int o = redis.hsetValue("friendlist", id, str);

        int n = redis.hsetValue("userinfo", id, buf);
        int m = redis.saddvalue("username", user.username);
        int o = redis.hsetValue("id_name", id, user.username);
        int p = redis.hsetValue("name_id", user.username, id);


        if (n == REDIS_REPLY_ERROR || m == REDIS_REPLY_ERROR || o == REDIS_REPLY_ERROR || p == REDIS_REPLY_ERROR)
        {
            cout << "注册失败" << endl;
            SendMsg sendmsg;
            sendmsg.SendMsg_int(fd, FAIL);
        }
        else
        {
            cout << "注册成功" << endl;
            SendMsg sendmsg;
            sendmsg.SendMsg_int(fd, SUCCESS);
            sendmsg.SendMsg_client(fd, id);
        }
    }
    return;
}

// 注销
void signout_server(int fd, string buf)
{
    json parsed_data = json::parse(buf);
    struct User user;
    user.id = parsed_data["id"];
    user.password = parsed_data["password"];
    printf("--- %s 用户将要注销 ---\n", user.id.c_str());
    printf("id：%s\n密码：%s\n", user.id.c_str(), user.password.c_str());

    Redis redis;
    redis.connect();

    if (redis.hashexists("userinfo", user.id) != 1) // 账号不存在
    {
        cout << "该用户名不存在，请注册 或 重新输入" << endl;
        SendMsg sendmsg;
        sendmsg.SendMsg_int(fd, USERNAMEUNEXIST);
    }
    else // 账号存在
    {
        string userjson_string;
        userjson_string = redis.gethash("userinfo", user.id);
        parsed_data = json::parse(userjson_string);
        // ************还要把该用户从所有人的好友名单、拉黑名单、群聊名单里删去**********
        // *********或者，当用户要看这些名单时，可以做一个判断，如果这个id存在，则打印出来，不存在，则不打印，但数据库来还存在就是了************
        if (user.password == parsed_data["password"] && redis.hashdel("userinfo", user.id) == 3 && redis.sremvalue("username", parsed_data["username"]) == 3 && redis.sremvalue("id_name",user.id) == 3 && redis.sremvalue("name_id",parsed_data["username"])) // 密码正确且id从哈希表中成功移除、姓名从昵称表里移除
        {
            cout << "注销成功" << endl;
            SendMsg sendmsg;
            sendmsg.SendMsg_int(fd, SUCCESS);
        }
        else
        {
            cout << "注销失败" << endl;
            SendMsg sendmsg;
            sendmsg.SendMsg_int(fd, FAIL);
        }
    }
    return;
}
