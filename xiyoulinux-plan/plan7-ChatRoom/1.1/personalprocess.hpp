// 服务器对客户端personalmenu里不同的选项进行不同的处理
#ifndef PERSONALPROCESS_HPP
#define PERSONALPROCESS_HPP

#include "data.h"
#include "define.h"
#include "IO.h"
#include "redis.hpp"
#include <vector>

#include <iostream>
using json = nlohmann::json;
using namespace std;

// 退出登录
void logout_server(string buf)
{
    json parsed_data = json::parse(buf);
    string id = parsed_data["id"];
    printf("--- %s 用户将要退出登录 ---\n", id.c_str());

    Redis redis;
    redis.connect();

    string userjson_string;
    userjson_string = redis.gethash("userinfo", id);
    parsed_data = json::parse(userjson_string);

    // 改变用户的在线状态
    parsed_data["online"] = OFFLINE;
    userjson_string = parsed_data.dump();
    redis.hsetValue("userinfo", id, userjson_string);
    redis.sremvalue("onlinelist", id); // 把用户从在线列表中移除

    cout << "退出登录成功" << endl;
}

// 加好友
void addfriend_server(int fd, string buf)
{
    json parsed_data = json::parse(buf);
    struct Friend friend_;
    friend_.id = parsed_data["id"];
    friend_.oppoid = parsed_data["oppoid"];
    printf("--- %s 用户将向 %s 发送好友申请 ---\n", friend_.id.c_str(), friend_.oppoid.c_str());

    Redis redis;
    redis.connect();

    // // 从数据库里取出好友列表
    // string userjson_string;
    // userjson_string = redis.gethash("friendlist", friend_.id);
    // parsed_data = json::parse(userjson_string);

    // // JSON 反序列化,并添加到 friendlist.friends 中
    // struct FriendList friendlist;
    // const auto &friendsJson = parsed_data["friends"];
    // for (const auto &friendName : friendsJson)
    // {
    //     friendlist.friends.push_back(friendName);
    // }

    // // 使用 find 在容器中查找目标 id
    // auto it = find(friendlist.friends.begin(), friendlist.friends.end(), friend_.oppoid);

    // 构造好友列表
    string key = friend_.id + ":friends";           // id+friends作为键，值就是id用户的好友们
    string key_ = friend_.oppoid + ":friends_apply"; // 对方的好友申请表

    // 加好友
    if (redis.hashexists("userinfo", friend_.oppoid) != 1) // 账号不存在
    {
        cout << "该id不存在，请重新输入" << endl;
        SendMsg sendmsg;
        sendmsg.SendMsg_int(fd, USERNAMEUNEXIST);
    }
    // else if (it != friendlist.friends.end())
    else if (redis.sismember(key, friend_.oppoid) == 1) // 好友列表里已有对方
    {
        cout << "你们已经是好友" << endl;
        SendMsg sendmsg;
        sendmsg.SendMsg_int(fd, HADFRIEND);
    }
    else if (redis.sismember("onlinelist", friend_.oppoid) == 1) // 在线列表里有对方
    {
        cout << "对方在线" << endl;

        // 放到对方的好友申请表中
        redis.saddvalue(key_, friend_.id);

        SendMsg sendmsg;
        sendmsg.SendMsg_int(fd, SUCCESS);
    }
    else // 对方不在线：加入数据库，等用户上线时提醒
    {
        cout << "对方不在线" << endl; //*******问不在线要怎么办*************

        // 放到对方的好友申请表中
        redis.saddvalue(key_, friend_.id);

        SendMsg sendmsg;
        sendmsg.SendMsg_int(fd, SUCCESS);
    }
}

// 好友申请
void friendapply_client(int fd, string buf)
{
    json parsed_data = json::parse(buf);
    struct Friend friend_;
    friend_.id = parsed_data["id"];
    printf("--- %s 用户查看好友申请 ---\n", friend_.id.c_str());

    Redis redis;
    redis.connect();

    string key = friend_.id + ":friends_apply";
    string userjson_string;
    int len = redis.scard(key);
    SendMsg sendmsg;
    sendmsg.SendMsg_int(fd, len);
    cout << "一共有 " << len << " 条好友请求" << endl;
    if (len == 0)
    {
        return;
    }

    redisReply **arry = redis.smembers(key);
    // 展示好友请求列表
    for (int i = 0; i < len; i++)
    {
        // 得到发送请求的用户id
        string applyfriend_id = arry[i]->str;

        // 根据id发送好友昵称
        nlohmann::json json_ = {
            {"username", redis.gethash("id_name", applyfriend_id)}, // 拿着id去找username
        };
        string json_string = json_.dump();
        SendMsg sendmsg;
        sendmsg.SendMsg_client(fd, json_string);

        // 接收客户端的判断
        int state_;
        RecvMsg recvmsg;
        state_ = recvmsg.RecvMsg_int(fd);
        printf("%d\n",state_);
        printf("%s\n",applyfriend_id.c_str());
        if (state_ == 1)
        {
            cout << "已同意" << endl;
            redis.sremvalue(key, applyfriend_id); // 从申请列表中移除
            string key1 = friend_.id + ":friends";
            string key2 = applyfriend_id + ":friends";
            redis.saddvalue(key1, applyfriend_id); // 对方成为自己好友
            redis.saddvalue(key2, friend_.id);     // 自己成为对方好友
        }
        else{
            cout << "已拒绝" << endl;
            redis.sremvalue(key, applyfriend_id); // 从申请列表中移除
        }
        freeReplyObject(arry[i]);
    }
}

#endif