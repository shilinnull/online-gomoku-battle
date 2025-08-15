# 项目名称

在线五子棋对抗

# 项目简介

实现五子棋服务器，能够让用户通过浏览器访问服务器，进行用户的注册，登录，对战匹配，实时对战，实时聊天等功能。

# 开发环境

Linux-ubuntu20.04，vim/vscode/g++/gdb/makefile

# 项目模块划分

数据管理模块：基于Mysql数据库进行用户数据的管理

前端界面模块：基于JS实现前端页面(注册，登录，游戏大厅，游戏房间)的动态控制以及与服务器的通信。

业务处理模块：搭建`WebSocket`服务器与客户端进行通信，接收请求并进行业务处理。提供用户通过浏览器进行用户注册，登录，以及实时匹配，对战，聊天等功能。

网络通信模块：基于websocketpp库实现Http&WebSocket服务器的搭建，提供网络通信功能。

会话管理模块：对客户端的连接进行cookie&session管理，实现http短连接时客户端身份识别功能。

在线管理模块：对进入游戏大厅与游戏房间中用户进行管理，提供用户是否在线以及获取用户连接的功能。

房间管理模块：为匹配成功的用户创建对战房间，提供实时的五子棋对战与聊天业务功能。

用户匹配模块：根据天梯分数不同进行不同层次的玩家匹配，为匹配成功的玩家创建房间并加入房间。

# 项目流程图

![](https://cdn.nlark.com/yuque/0/2025/png/36043961/1755236510179-397fc8ac-0157-491e-b58a-c0361ee2f61b.png)![](https://cdn.nlark.com/yuque/0/2025/png/36043961/1755236510296-4952e3fc-78fd-456d-a288-aea828cbf135.png)

![](https://cdn.nlark.com/yuque/0/2025/png/36043961/1755236510397-7416451a-27f4-4cc9-b79b-36b147c1e146.png)

# 服务端开发

## 数据管理模块

基于mysql数据库进行数据管理以及封装数据管理模块实现数据库访问。

基于Json进行数据的序列化和反序列化。

对字符串进行切割提取

对文件进行读取

### 实现工具库类

```cpp
#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <mysql/mysql.h>
#include <jsoncpp/json/json.h>

#include "Logger.hpp"

class mysql_util
{
public:
static MYSQL *mysql_create(const std::string &host,
const std::string &username,
const std::string &password,
const std::string &dbname,
uint16_t port = 3306)
{
    MYSQL *mysql = mysql_init(NULL);
    if (mysql == NULL)
    {
        ELOG("mysql init failed!");
        return NULL;
    }
    // 2. 连接服务器
    if (mysql_real_connect(mysql,
        host.c_str(),
        username.c_str(),
        password.c_str(),
        dbname.c_str(), port, NULL, 0) == NULL)
    {
        ELOG("connect mysql server failed : %s", mysql_error(mysql));
        mysql_close(mysql);
        return NULL;
    }
    // 3. 设置客户端字符集
    if (mysql_set_character_set(mysql, "utf8") != 0)
    {
        ELOG("set client character failed : %s", mysql_error(mysql));
        mysql_close(mysql);
        return NULL;
    }
    return mysql;
}
static bool mysql_exec(MYSQL *mysql, const std::string &sql)
{
    int ret = mysql_query(mysql, sql.c_str());
    if (ret != 0)
    {
        ELOG("%s\n", sql.c_str());
        ELOG("mysql query failed : %s\n", mysql_error(mysql));
        return false;
    }
    return true;
}
static void mysql_destroy(MYSQL *mysql)
{
    if (mysql != NULL)
    {
        mysql_close(mysql);
    }
    return;
}
};

class json_util
{
public:
static bool serialize(const Json::Value &root, std::string *str)
{
    Json::StreamWriterBuilder swb;
    std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
    std::stringstream ss;
    int ret = sw->write(root, &ss);
    if (ret != 0)
    {
        ELOG("json serialize failed!!");
        return false;
    }
    *str = ss.str();
    return true;
}
static bool unserialize(const std::string &str, Json::Value *root)
{
    Json::CharReaderBuilder crb;
    std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
    std::string err;
    bool ret = cr->parse(str.c_str(), str.c_str() + str.size(), root, &err);
    if (ret == false)
    {
        ELOG("json unserialize failed: %s", err.c_str());
        return false;
    }
    return true;
}
};

class string_util
{
public:
static int split(const std::string &src, const std::string &sep, std::vector<std::string> &res)
{
    for (int index = 0; index < src.size();)
        {
            size_t pos = src.find(sep, index); // 找分隔符
            if (pos == std::string::npos) // 找到结尾了
            {
                res.push_back(src.substr(index));
                break;
            }
            if (pos == index) // 当前位置 需要跳过sep
            {
                index += sep.size();
                continue;
            }
            res.push_back(src.substr(index, pos - index));
            index = pos + sep.size(); // 截取下一个
        }
        return res.size();
    }
};

class file_util
{
public:
    static bool read(const std::string &filename, std::string &body)
    {
        // 打开文件
        std::ifstream ifs(filename, std::ios::binary);
        if (ifs.is_open() == false)
        {
            ELOG("%s file open failed!!", filename.c_str());
            return false;
        }
        // 获取文件大小
        size_t fsize = 0;
        ifs.seekg(0, std::ios::end);
        fsize = ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        body.resize(fsize);
        // 将文件所有数据读取出来
        ifs.read(&body[0], fsize);
        if (ifs.good() == false)
        {
            ELOG("read %s file content failed!", filename.c_str());
            ifs.close();
            return false;
        }
        // 关闭文件
        ifs.close();
        return true;
    }
};

#endif
```

### 用户表的设计

```sql
create database if not exists gobang;

use gobang;

create table if not exists user(
  id int primary key auto_increment,
  username varchar(32) unique key not null,
  password varchar(32) not null,
  score int,
  total_count int,
  win_count int
);
```

### 管理用户数据

该类的作用是负责通过 MySQL 接口管理用户数据。主要提供了四个方法:

select_by_name：根据用户名查找用户信息，用于实现登录功能

insert：新增用户，用户实现注册功能

login：登录验证，并获取完整的用户信息

win：用于给获胜玩家修改分数

lose：用户给失败玩家修改分数

### 类的设计

```cpp
#ifndef __DB_HPP__
#define __DB_HPP__

#include "util.hpp"
#include <mutex>
#include <cassert>

class user_table
{
private:
MYSQL *_mysql;
std::mutex _mutex;
public:
user_table(const std::string &host,
const std::string &username,
const std::string &password,
const std::string &dbname,
uint16_t port = 330)
{
    _mysql = mysql_util::mysql_create(host, username, password, dbname, port);
    assert(_mysql != nullptr);
}
~user_table()
{
    mysql_util::mysql_destroy(_mysql);
    _mysql = nullptr;
}

// 注册时新增用户
bool insert(Json::Value &user)
{
}

// 登录验证，并返回详细的用户信息
bool login(Json::Value &user)
{
}
// 通过用户名获取用户信息
bool select_by_name(const std::string &name, Json::Value &user)
{
}

// 通过用户名获取用户信息
bool select_by_id(uint64_t id, Json::Value &user)
{
}

// 胜利时天梯分数增加30分，战斗场次增加1，胜利场次增加1
bool win(uint64_t id)
{
}

// 失败时天梯分数减少30，战斗场次增加1，其他不变
bool lose(uint64_t id)
{
}
};
#endif
```

### 类的实现

```cpp
#ifndef __DB_HPP__
#define __DB_HPP__

#include "util.hpp"
#include <mutex>
#include <cassert>

class user_table
{
private:
MYSQL *_mysql;
std::mutex _mutex;
public:
user_table(const std::string &host,
const std::string &username,
const std::string &password,
const std::string &dbname,
uint16_t port = 330)
{
    _mysql = mysql_util::mysql_create(host, username, password, dbname, port);
    assert(_mysql != nullptr);
}
~user_table()
{
    mysql_util::mysql_destroy(_mysql);
    _mysql = nullptr;
}

// 注册时新增用户
bool insert(Json::Value &user)
{
    #define INSERT_USER "insert user values(null, '%s', '%s', 1000, 0, 0);"
    if (user["username"].isNull() || user["password"].isNull())
    {
        DLOG("INPUT PASSWORD OR USERNAME");
        return false;
    }
    char sql[4096] = {0};
    sprintf(sql, INSERT_USER, user["username"].asCString(), user["password"].asCString());
    bool ret = mysql_util::mysql_exec(_mysql, sql);
    if (!ret)
    {
        DLOG("insert user info failed!!\n");
        return false;
    }
    return true;
}

// 登录验证，并返回详细的用户信息
bool login(Json::Value &user)
{
    if (user["password"].isNull() || user["username"].isNull())
    {
    DLOG("INPUT PASSWORD OR USERNAME");
    return false;
}

    #define LOGIN_USER "select id, score, total_count, win_count from user where username='%s' and password='%s';"
    char sql[4096] = {0};
    sprintf(sql, LOGIN_USER, user["username"].asCString(), user["password"].asCString());
    MYSQL_RES *res = nullptr;

    { // 线程安全的
        std::unique_lock<std::mutex> lock(_mutex);
        bool ret = mysql_util::mysql_exec(_mysql, sql); // 执行
        if (!ret)
        {
            DLOG("user login failed!!\n");
            return false;
        }
        res = mysql_store_result(_mysql); // 查询的结果保存到res
        if (!res)
        {
            DLOG("have no login user info!!");
            return false;
        }
    }

    int row_num = mysql_num_rows(res);
    if (row_num != 1)
    {
        DLOG("the user information queried is not unique!!");
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    user["id"] = (Json::UInt64)std::stol(row[0]);
    user["score"] = (Json::UInt64)std::stol(row[1]);
    user["total_count"] = (Json::UInt64)std::stol(row[2]);
    user["win_count"] = (Json::UInt64)std::stol(row[3]);
    mysql_free_result(res);
    return true;
}
// 通过用户名获取用户信息
bool select_by_name(const std::string &name, Json::Value &user)
{
    #define USER_BY_NAME "select id, score, total_count, win_count from user where username='%s';"
    char sql[4096] = {0};
    sprintf(sql, USER_BY_NAME, name.c_str());
    MYSQL_RES *res = nullptr;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            bool ret = mysql_util::mysql_exec(_mysql, sql);
            if (ret == false)
            {
                DLOG("get user by name failed!!\n");
                return false;
            }
            res = mysql_store_result(_mysql);
            if (res == nullptr)
            {
                DLOG("have no user info!!");
                return false;
            }
        }
        int row_num = mysql_num_rows(res);
        if (row_num != 1)
        {
            DLOG("the user information queried is not unique!!");
            return false;
        }
        MYSQL_ROW row = mysql_fetch_row(res);
        user["id"] = (Json::UInt64)std::stol(row[0]);
        user["username"] = name;
        user["score"] = (Json::UInt64)std::stol(row[1]);
        user["total_count"] = std::stoi(row[2]);
        user["win_count"] = std::stoi(row[3]);
        mysql_free_result(res);
        return true;
    }
    
    // 通过用户名获取用户信息
    bool select_by_id(uint64_t id, Json::Value &user)
    {
#define USER_BY_ID "select username, score, total_count, win_count from user where id=%ld;"
        char sql[4096] = {0};
        sprintf(sql, USER_BY_ID, id);
        MYSQL_RES *res = nullptr;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            bool ret = mysql_util::mysql_exec(_mysql, sql);
            if (ret == false)
            {
                DLOG("get user by id failed!!\n");
                return false;
            }
            res = mysql_store_result(_mysql);
            if (res == nullptr)
            {
                DLOG("have no user info!!");
                return false;
            }
        }
        int row_num = mysql_num_rows(res);
        if (row_num != 1)
        {
            DLOG("the user information queried is not unique!!");
            return false;
        }
        MYSQL_ROW row = mysql_fetch_row(res);
        user["id"] = (Json::UInt64)id;
        user["username"] = row[0];
        user["score"] = (Json::UInt64)std::stol(row[1]);
        user["total_count"] = std::stoi(row[2]);
        user["win_count"] = std::stoi(row[3]);
        mysql_free_result(res);
        return true;
    }

    // 胜利时天梯分数增加30分，战斗场次增加1，胜利场次增加1
    bool win(uint64_t id)
    {
#define USER_WIN "update user set score=score+30, total_count=total_count+1, win_count=win_count+1 where id=%ld;"
        char sql[4096] = {0};
        sprintf(sql, USER_WIN, id);
        bool ret = mysql_util::mysql_exec(_mysql, sql);
        if (ret == false)
        {
            DLOG("update win user info failed!!\n");
            return false;
        }
        return true;
    }

    // 失败时天梯分数减少30，战斗场次增加1，其他不变
    bool lose(uint64_t id)
    {
#define USER_LOSE "update user set score=score-30, total_count=total_count+1 where id=%ld;"
        char sql[4096] = {0};
        sprintf(sql, USER_LOSE, id);
        bool ret = mysql_util::mysql_exec(_mysql, sql);
        if (ret == false)
        {
            DLOG("update lose user info failed!!\n");
            return false;
        }
        return true;
    }
};
#endif
```

## session管理模块

设计一个session类，但是session对象不能一直存在，这样是一种资源泄漏，因此需要使用定时器对每个创建的session对象进行定时销毁（一个客户端连接断开后，一段时间内都没有重新连接则销毁session）

`_ssid`使用时间戳填充。实际上，我们通常使用唯一id生成器生成一个唯一的id

`_user`保存当前用户的信息

`timer_ptr_tp`保存当前`session`对应的定时销毁任务

### 类的实现

```cpp
#ifndef __SESSION_HPP__
#define __SESSION_HPP__

#include "util.hpp"
#include <unordered_map>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>

typedef enum
{
UNLOGIN,
LOGIN
} ss_statu;

class session
{
public:
session(uint64_t ssid) : _ssid(ssid) { DLOG("SESSION %p 被创建！！", this); }
~session() { DLOG("SESSION %p 被释放！！", this); }
uint64_t ssid() { return _ssid; }
void set_statu(ss_statu statu) { _statu = statu; }
void set_user(uint64_t uid) { _uid = uid; }
uint64_t get_user() { return _uid; }
bool is_login() { return (_statu == LOGIN); }
void set_timer(const wsserver_t::timer_ptr &tp) { _tp = tp; }
wsserver_t::timer_ptr &get_timer() { return _tp; }

private:
uint64_t _ssid;            // 唯一标识符
uint64_t _uid;             // session对应的用户id
ss_statu _statu;           // 用户状态：未登录，已登录
wsserver_t::timer_ptr _tp; // session关联的定时器
};

#endif
```

封装session管理，实现http客户端通信状态的维护及身份识别。

### session管理

1. 创建一个新的session
2. 通过ssid获取session
3. 通过ssid判断session是否存在
4. 销毁session
5. 为session设置过期时间，过期后session被销毁

设置过期时间的时候依赖于websocketpp的定时器来完成session生命周期的管理。 登录之后，创建session，session需要在指定时间无通信后删除 是进入游戏大厅，或者游戏房间，这个session就应该永久存在 等到退出游戏大厅，或者游戏房间，这个session应该被重新设置为临时，在长时间无通信后被删除

### 类的设计

```cpp
#define SESSION_TIMEOUT 30000
#define SESSION_FOREVER -1
using session_ptr = std::shared_ptr<session>;

class session_manager
{
public:
session_manager(wsserver_t *srv)
{

}
~session_manager() { }

session_ptr create_session(uint64_t uid, ss_statu statu)
{
}

void append_session(const session_ptr &ssp)
{
}

session_ptr get_session_by_ssid(uint64_t ssid)
{
}
void remove_session(uint64_t ssid)
{
}
void set_session_expire_time(uint64_t ssid, int ms)
{
}

private:
uint64_t _next_ssid;
std::mutex _mutex;
std::unordered_map<uint64_t, session_ptr> _session;
wsserver_t *_server;
};
```

### 类的实现

```cpp
#define SESSION_TIMEOUT 30000
#define SESSION_FOREVER -1
using session_ptr = std::shared_ptr<session>;

class session_manager
{
public:
session_manager(wsserver_t *srv)
: _next_ssid(1), _server(srv)
{
    DLOG("session管理器初始化完毕！");
}
~session_manager() { DLOG("session管理器即将销毁！"); }

session_ptr create_session(uint64_t uid, ss_statu statu)
{
    std::unique_lock<std::mutex> _lock;
    session_ptr ssp(new session(_next_ssid++)); // 创建一个session
    ssp->set_statu(statu);
    ssp->set_user(uid);

    _session.insert(std::make_pair(ssp->ssid(), ssp)); // 添加进行管理
    return ssp;
}

void append_session(const session_ptr &ssp)
{
    std::unique_lock<std::mutex> _lock;
    _session.insert(std::make_pair(ssp->ssid(), ssp));
}

session_ptr get_session_by_ssid(uint64_t ssid)
{
    std::unique_lock<std::mutex> lock(_mutex);
    auto it = _session.find(ssid);
    if (it == _session.end())
        return session_ptr();
    return it->second;
}
void remove_session(uint64_t ssid)
{
    std::unique_lock<std::mutex> lock(_mutex);
    _session.erase(ssid);
}
void set_session_expire_time(uint64_t ssid, int ms)
{
    session_ptr ssp = get_session_by_ssid(ssid);
    if (ssp.get() == nullptr)
        return;
    wsserver_t::timer_ptr tp = ssp->get_timer();
    if (tp.get() == nullptr && ms == SESSION_FOREVER)
    {
        // 1. 在session永久存在的情况下，设置永久存在
        return;
    }
    else if (tp.get() == nullptr && ms != SESSION_FOREVER)
    {
        // 2. 在session永久存在的情况下，设置指定时间之后被删除的定时任务
        wsserver_t::timer_ptr tmp_tp = _server->set_timer(ms,
        std::bind(&session_manager::remove_session, this, ssid));
        ssp->set_timer(tmp_tp);
    }
    else if (tp.get() != nullptr && ms == SESSION_FOREVER)
    {
        // 3. 在session设置了定时删除的情况下，将session设置为永久存在
        // 删除定时任务--- stready_timer删除定时任务会导致任务直接被执行
        tp->cancel(); // 不会立即取消

        // 需要重新给session管理器中，添加一个session信息, 且添加的时候需要使用定时器，而不是立即添加
        ssp->set_timer(wsserver_t::timer_ptr()); // 将session关联的定时器设置为空
        _server->set_timer(0, std::bind(&session_manager::append_session, this, ssp));
    }
    else if (tp.get() != nullptr && ms != SESSION_FOREVER)
    {
        // 4. 在session设置了定时删除的情况下，将session重置删除时间
        tp->cancel(); // 因为这个取消定时任务并不是立即取消的
        ssp->set_timer(wsserver_t::timer_ptr());
        _server->set_timer(0, std::bind(&session_manager::append_session, this, ssp));

        // 重新给session添加定时销毁任务
        wsserver_t::timer_ptr tmp_tp = _server->set_timer(ms,
        std::bind(&session_manager::remove_session, this, ssp->ssid()));
        // 重新设置session关联的定时器
        ssp->set_timer(tmp_tp);
    }
    else
    {
        ELOG("未知错误!");
    }
}

private:
uint64_t _next_ssid;
    std::mutex _mutex;
    std::unordered_map<uint64_t, session_ptr> _session;
    wsserver_t *_server;
};
```

## 在线用户管理模块

对于进入游戏大厅&游戏房间的长连接通信进行管理，实现随时能够获取客户端连接进行消息的主动推送。

管理的是两类用户：进入游戏大厅的&进入游戏房间的

原因：进入游戏大厅的用户和进入游戏房间的用户才会建立websocket长连接

管理：将用户id和对应的客户端websocket长连接关联起来

作用：当一个用户发送了消息（实时聊天消息/下棋消息），我们可以找到房间中的其他用户，在在线用户管理模块中，找到这个用户对应的websocket连接，然后将消息发送给指定的用户

1. 通过用户ID找到用户连接，进而实现向指定用户的客户端推送消息websocket连接关闭时，会自动在在线用户管理模块中删除自己的信息
2. 可以通过判断一个用户是否还在用户管理模块中来确定用户是否在线。

### 类的设计

```cpp
#ifndef __ONLINE_HPP__
#define __ONLINE_HPP__

#include "util.hpp"
#include <mutex>
#include <unordered_map>

class online_manager
{
public:
// websocket连接建立的时候才会加入游戏大厅&游戏房间在线用户管理
void enter_game_hall(uint64_t uid, wsserver_t::connection_ptr &conn)
{
}
void enter_game_room(uint64_t uid, wsserver_t::connection_ptr &conn)
{
}

// websocket连接断开的时候，才会移除游戏大厅&游戏房间在线用户管理
void exit_game_hall(uint64_t uid)
{
}
void exit_game_room(uint64_t uid)
{
}

// 判断当前指定用户是否在游戏大厅/游戏房间
bool is_in_game_hall(uint64_t uid)
{
}
bool is_in_game_room(uint64_t uid)
{
}

// 通过用户ID在游戏大厅/游戏房间用户管理中获取对应的通信连接
wsserver_t::connection_ptr get_conn_from_hall(uint64_t uid)
{
}
wsserver_t::connection_ptr get_conn_from_room(uint64_t uid)
{
}
private:
std::mutex _mutex;
// 用于建立游戏大厅用户的用户ID与通信连接的关系
std::unordered_map<uint64_t, wsserver_t::connection_ptr> _hall_user;
// 用于建立游戏房间用户的用户ID与通信连接的关系
std::unordered_map<uint64_t, wsserver_t::connection_ptr> _room_user;
};

#endif
```

### 类的实现

```cpp
#ifndef __ONLINE_HPP__
#define __ONLINE_HPP__

#include "util.hpp"
#include <mutex>
#include <unordered_map>

class online_manager
{
public:
// websocket连接建立的时候才会加入游戏大厅&游戏房间在线用户管理
void enter_game_hall(uint64_t uid, wsserver_t::connection_ptr &conn)
{
    std::unique_lock<std::mutex> _lock;
    _hall_user.insert(std::make_pair(uid, conn));
}
void enter_game_room(uint64_t uid, wsserver_t::connection_ptr &conn)
{
    std::unique_lock<std::mutex> _lock;
    _room_user.insert(std::make_pair(uid, conn));
}

// websocket连接断开的时候，才会移除游戏大厅&游戏房间在线用户管理
void exit_game_hall(uint64_t uid)
{
    std::unique_lock<std::mutex> _lock;
    _hall_user.erase(uid);
}
void exit_game_room(uint64_t uid)
{
    std::unique_lock<std::mutex> _lock;
    _room_user.erase(uid);
}

// 判断当前指定用户是否在游戏大厅/游戏房间
bool is_in_game_hall(uint64_t uid)
{
    std::unique_lock<std::mutex> lock(_mutex);
    auto pos = _hall_user.find(uid);
    if (pos == _hall_user.end())
    {
        return false;
    }
    return true;
}
bool is_in_game_room(uint64_t uid)
{
    std::unique_lock<std::mutex> lock(_mutex);
    auto pos = _room_user.find(uid);
    if (pos == _room_user.end())
    {
        return false;
    }
    return true;
}

// 通过用户ID在游戏大厅/游戏房间用户管理中获取对应的通信连接
wsserver_t::connection_ptr get_conn_from_hall(uint64_t uid)
{
    std::unique_lock<std::mutex> lock(_mutex);
    auto it = _hall_user.find(uid);
    if (it == _hall_user.end())
    {
        return wsserver_t::connection_ptr();
    }
    return it->second;
}
wsserver_t::connection_ptr get_conn_from_room(uint64_t uid)
{
    std::unique_lock<std::mutex> lock(_mutex);
    auto it = _room_user.find(uid);
    if (it == _room_user.end())
    {
        return wsserver_t::connection_ptr();
    }
    return it->second;
}
private:
std::mutex _mutex;
// 用于建立游戏大厅用户的用户ID与通信连接的关系
std::unordered_map<uint64_t, wsserver_t::connection_ptr> _hall_user;
// 用于建立游戏房间用户的用户ID与通信连接的关系
std::unordered_map<uint64_t, wsserver_t::connection_ptr> _room_user;
};

#endif
```

## 游戏房间管理

对于同一个房间中的用户及动作进行处理(对战匹配，下棋，聊天，退出)。

首先，需要设计一个房间类，能够实现房间的实例化，房间类主要是对匹配成对的玩家建立一个小范围的关联关系，一个房间中任意一个用户发生的任何动作，都会被广播给房间中的其他用户。

而房间中的动作主要包含两类：

1. 棋局对战
2. 实时聊天

### 类的设计

```cpp
#ifndef __ROOM_HPP__
#define __ROOM_HPP__

#include "util.hpp"
#include "online.hpp"
#include "db.hpp"

#define BOARD_ROW 15  // 棋盘行
#define BOARD_COL 15  // 棋盘列
#define CHESS_WHITE 1 // 白棋子
#define CHESS_BLACK 2 // 黑棋子

// 房间状态
typedef enum
{
GAME_START,
GAME_OVER
} room_statu;

class room
{
private:
bool five(int row, int col, int row_off, int col_off, int color)
{
}
int check_win(int row, int col, int color)
{
}

public:
room(uint64_t room_id, user_table *tb_user, online_manager *online_user)
: _room_id(room_id), _statu(GAME_START), _player_count(0),
_tb_user(tb_user), _online_user(online_user), _board(BOARD_ROW, std::vector<int>(BOARD_COL, 0))

{
}
~room()
{
}
uint64_t id() {  }
room_statu statu() {  }
int player_count() {  }
void add_white_user(uint64_t uid)
{
}
void add_black_user(uint64_t uid)
{
}
uint64_t get_white_user() {  }
uint64_t get_black_user() {  }

/*总的请求处理函数，在函数内部，区分请求类型，根据不同的请求调用不同的处理函数，得到响应进行广播*/
void handle_request(Json::Value &req)
{
}

/*处理下棋动作*/
Json::Value handle_chess(Json::Value &req)
{
}

/*处理聊天动作*/
Json::Value handle_chat(Json::Value &req)
{
}

/*处理玩家退出房间动作*/
void handle_exit(uint64_t uid)
{
}

/*将指定的信息广播给房间中所有玩家*/
void broadcast(Json::Value &rsp)
{
}

private:
uint64_t _room_id; // 房间id
room_statu _statu; // 房间状态
int _player_count;
uint64_t _white_id;
uint64_t _black_id;
user_table *_tb_user;                 // 用户管理
online_manager *_online_user;         // 在线用户管理
std::vector<std::vector<int>> _board; // 棋盘大小
};

#endif
```

### 类的实现

```cpp
#ifndef __ROOM_HPP__
#define __ROOM_HPP__

#include "util.hpp"
#include "online.hpp"
#include "db.hpp"

#define BOARD_ROW 15  // 棋盘行
#define BOARD_COL 15  // 棋盘列
#define CHESS_WHITE 1 // 白棋子
#define CHESS_BLACK 2 // 黑棋子

// 房间状态
typedef enum
{
GAME_START,
GAME_OVER
} room_statu;

class room
{
private:
bool five(int row, int col, int row_off, int col_off, int color)
{
    // row和col是下棋位置，  row_off和col_off是偏移量，也是方向
    int count = 1;
    int search_row = row + row_off;
    int search_col = col + col_off;
    while (search_row >= 0 && search_row < BOARD_ROW &&
        search_col >= 0 && search_col < BOARD_COL &&
        _board[search_row][search_col] == color)
        {
            // 同色棋子数量++
            count++;

            // 检索位置继续向后偏移
            search_row += row_off;
            search_col += col_off;
        }
    search_row = row - row_off;
    search_col = col - col_off;

    while (search_row >= 0 && search_row < BOARD_ROW &&
        search_col >= 0 && search_col < BOARD_COL &&
        _board[search_row][search_col] == color)
        {
            // 同色棋子数量++
            count++;
            // 检索位置继续向后偏移
            search_row -= row_off;
            search_col -= col_off;
        }
    return count >= 5;
}
uint64_t check_win(int row, int col, int color)
{
    // 从下棋位置的四个不同方向上检测是否出现了5个及以上相同颜色的棋子（横行，纵列，正斜，反斜）
    if (five(row, col, 0, 1, color) ||
        five(row, col, 1, 0, color) ||
        five(row, col, -1, -1, color) ||
        five(row, col, 1, -1, color))

    {
        //任意一个方向上出现了true也就是五星连珠，则设置返回值
        return color == CHESS_WHITE ? _white_id : _black_id;
    }
    return 0;
}

public:
room(uint64_t room_id, user_table *tb_user, online_manager *online_user)
: _room_id(room_id), _statu(GAME_START), _player_count(0),
_tb_user(tb_user), _online_user(online_user), _board(BOARD_ROW, std::vector<int>(BOARD_COL, 0))

{
    DLOG("%lu 房间创建成功!!", _room_id);
}
~room()
{
    DLOG("%lu 房间销毁成功!!", _room_id);
}
uint64_t id() { return _room_id; }
room_statu statu() { return _statu; }
int player_count() { return _player_count; }
void add_white_user(uint64_t uid)
{
    _white_id = uid;
    _player_count++;
}
void add_black_user(uint64_t uid)
{
    _black_id = uid;
    _player_count++;
}
uint64_t get_white_user() { return _white_id; }
uint64_t get_black_user() { return _black_id; }

/*总的请求处理函数，在函数内部，区分请求类型，根据不同的请求调用不同的处理函数，得到响应进行广播*/
void handle_request(Json::Value &req)
{
    // 1. 校验房间号是否匹配
    Json::Value json_resp;
    uint64_t room_id = req["room_id"].asUInt64();
    if (room_id != _room_id)
    {
        json_resp["optype"] = req["optype"].asString();
        json_resp["result"] = false;
        json_resp["reason"] = "房间号不匹配！";
        return broadcast(json_resp);
        }
        // 2. 根据不同的请求类型调用不同的处理函数
        if (req["optype"].asString() == "put_chess") // 放置棋子
        {
            json_resp = handle_chess(req);
            if (json_resp["winner"].asUInt64() != 0)
            {
                uint64_t winner_id = json_resp["winner"].asUInt64();
                uint64_t loser_id = winner_id == _white_id ? _black_id : _white_id;
                _tb_user->win(winner_id);
                _tb_user->lose(loser_id);
                _statu = GAME_OVER;
            }
        }
        else if (req["optype"].asString() == "chat") // 聊天
            json_resp = handle_chat(req);
        else // 错误请求
        {
            json_resp["optype"] = req["optype"].asString();
            json_resp["result"] = false;
            json_resp["reason"] = "未知请求类型";
        }

        std::string body;
        json_util::serialize(json_resp, &body);
        DLOG("房间-广播动作: %s", body.c_str());
        broadcast(json_resp);
        return;
    }

    /*处理下棋动作*/
    Json::Value handle_chess(Json::Value &req)
    {
        Json::Value json_resp = req;
        // 判断房间中两个玩家是否都在线，任意一个不在线，就是另一方胜利。
        int chess_row = req["row"].asInt();
        int chess_col = req["col"].asInt();
        uint64_t cur_uid = req["uid"].asUInt64();
        if (_online_user->is_in_game_room(_white_id) == false)
        {
            json_resp["result"] = true;
            json_resp["reason"] = "运气真好！对方掉线，不战而胜！";
            json_resp["winner"] = (Json::UInt64)_black_id;
            return json_resp;
        }
        if (_online_user->is_in_game_room(_black_id) == false)
        {
            json_resp["result"] = true;
            json_resp["reason"] = "运气真好！对方掉线，不战而胜！";
            json_resp["winner"] = (Json::UInt64)_white_id;
            return json_resp;
        }
        // 获取走棋位置，判断当前走棋是否合理（位置是否已经被占用）
        if (_board[chess_row][chess_col] != 0)
        {
            json_resp["result"] = false;
            json_resp["reason"] = "当前位置已经有了其他棋子！";
            return json_resp;
        }
        int cur_color = cur_uid == _white_id ? CHESS_WHITE : CHESS_BLACK;
        _board[chess_row][chess_col] = cur_color;
        // 判断是否有玩家胜利（从当前走棋位置开始判断是否存在五星连珠）
        uint64_t winner_id = check_win(chess_row, chess_col, cur_color);
        if (winner_id != 0)
            json_resp["reason"] = "五星连珠，战无敌！";
        json_resp["result"] = true;
        json_resp["winner"] = (Json::UInt64)winner_id;
        return json_resp;
    }

    /*处理聊天动作*/
    Json::Value handle_chat(Json::Value &req)
    {
        Json::Value json_resp = req;
        // 检测消息中是否包含敏感词
        std::string msg = req["message"].asString();
        auto pos = msg.find("垃圾");
        if (pos != std::string::npos)
        {
            json_resp["result"] = false;
            json_resp["reason"] = "消息中包含敏感词，不能发送！";
            return json_resp;
        }
        json_resp["result"] = true;
        return json_resp;
    }

    /*处理玩家退出房间动作*/
    void handle_exit(uint64_t uid)
    {
        // 如果是下棋中退出，则对方胜利，否则下棋结束了退出，则是正常退出
        Json::Value json_resp;
        if (_statu == GAME_START)
        {
            uint64_t winner_id = (Json::UInt64)(uid == _white_id ? _black_id : _white_id);
            uint64_t loser_id = winner_id == _white_id ? _black_id : _white_id;

            json_resp["optype"] = "put_chess";
            json_resp["result"] = true;
            json_resp["reason"] = "对方掉线，不战而胜！";
            json_resp["room_id"] = (Json::UInt64)_room_id;
            json_resp["uid"] = (Json::UInt64)uid;
            json_resp["row"] = -1;
            json_resp["col"] = -1;
            json_resp["winner"] = (Json::UInt64)winner_id;

            _tb_user->win(winner_id); // 设置赢家
            _tb_user->lose(loser_id); // 设置输家
            _statu = GAME_OVER;       // 设置游戏状态
            broadcast(json_resp);
        }
        // 房间中玩家数量--
        _player_count--;
        return;
    }

    /*将指定的信息广播给房间中所有玩家*/
    void broadcast(Json::Value &rsp)
    {
        // 1. 对要响应的信息进行序列化，将Json::Value中的数据序列化成为json格式字符串
        std::string body;
        json_util::serialize(rsp, &body);
        // 2. 获取房间中所有用户的通信连接
        // 3. 发送响应信息
        wsserver_t::connection_ptr wconn = _online_user->get_conn_from_room(_white_id);
        if (wconn.get() != nullptr)
            wconn->send(body);
        else
            DLOG("房间-白棋玩家连接获取失败");
        wsserver_t::connection_ptr bconn = _online_user->get_conn_from_room(_black_id);
        if (bconn.get() != nullptr)
            bconn->send(body);
        else
            DLOG("房间-黑棋玩家连接获取失败");
        return;
    }

private:
    uint64_t _room_id; // 房间id
    room_statu _statu; // 房间状态
    int _player_count;
    uint64_t _white_id;
    uint64_t _black_id;
    user_table *_tb_user;                 // 用户管理
    online_manager *_online_user;         // 在线用户管理
    std::vector<std::vector<int>> _board; // 棋盘大小
};

#endif
```

### room房间管理

### 类的设计

```cpp
using room_ptr = std::shared_ptr<room>;

class room_manager
{
public:
/*初始化房间ID计数器*/
room_manager(user_table *ut, online_manager *om)
: _next_rid(1), _tb_user(ut), _online_user(om)
{
}

~room_manager()
{
}
// 为两个用户创建房间，并返回房间的智能指针管理对象
room_ptr create_room(uint64_t uid1, uint64_t uid2)
{
}

/*通过房间ID获取房间信息*/
room_ptr get_room_by_rid(uint64_t rid)
{
}

/*通过用户ID获取房间信息*/
room_ptr get_room_by_uid(uint64_t uid)
{
}
/*通过房间ID销毁房间*/
void remove_room(uint64_t rid)
{
}

/*删除房间中指定用户，如果房间中没有用户了，则销毁房间，用户连接断开时被调用*/
void remove_room_user(uint64_t uid)
{
}

private:
uint64_t _next_rid; // 房间的id
std::mutex _mutex;
user_table *_tb_user;
online_manager *_online_user;
std::unordered_map<uint64_t, room_ptr> _rooms; // 房间信息管理
std::unordered_map<uint64_t, uint64_t> _users; // 两个对战的用户管理
};
```

### 类的实现

```cpp
#ifndef __ROOM_HPP__
#define __ROOM_HPP__

#include "util.hpp"
#include "online.hpp"
#include "db.hpp"

#define BOARD_ROW 15  // 棋盘行
#define BOARD_COL 15  // 棋盘列
#define CHESS_WHITE 1 // 白棋子
#define CHESS_BLACK 2 // 黑棋子

// 房间状态
typedef enum
{
GAME_START,
GAME_OVER
} room_statu;

using room_ptr = std::shared_ptr<room>;

class room_manager
{
public:
/*初始化房间ID计数器*/
room_manager(user_table *ut, online_manager *om)
: _next_rid(1), _tb_user(ut), _online_user(om)
{
    DLOG("房间管理模块初始化完毕！");
}

~room_manager()
{
    DLOG("房间管理模块即将销毁！");
}
// 为两个用户创建房间，并返回房间的智能指针管理对象
room_ptr create_room(uint64_t uid1, uint64_t uid2)
{
    // 两个用户在游戏大厅中进行对战匹配，匹配成功后创建房间
    // 1. 校验两个用户是否都还在游戏大厅中，只有都在才需要创建房间。
    if (_online_user->is_in_game_hall(uid1) == false)
    {
        DLOG("用户：%lu 不在大厅中，创建房间失败!", uid1);
        return room_ptr();
    }
    if (_online_user->is_in_game_hall(uid2) == false)
    {
        DLOG("用户：%lu 不在大厅中，创建房间失败!", uid2);
        return room_ptr();
    }
    // 2. 创建房间，将用户信息添加到房间中
    std::unique_lock<std::mutex> lock(_mutex);
    room_ptr rp(new room(_next_rid, _tb_user, _online_user));
    rp->add_white_user(uid1);
    rp->add_black_user(uid2);
    // 3. 将房间信息管理起来
    _rooms.insert(std::make_pair(_next_rid, rp));
    _users.insert(std::make_pair(uid1, _next_rid));
    _users.insert(std::make_pair(uid2, _next_rid));
    _next_rid++; // 准备下一个房间id
    return rp;
}

/*通过房间ID获取房间信息*/
room_ptr get_room_by_rid(uint64_t rid)
{
    std::unique_lock<std::mutex>(_mutex);
    auto it = _rooms.find(rid);
    if (it == _rooms.end())
        return room_ptr();
    return it->second;
}

/*通过用户ID获取房间信息*/
room_ptr get_room_by_uid(uint64_t uid)
{
    std::unique_lock<std::mutex> lock(_mutex);
    // 1. 通过用户ID获取房间ID
    auto uit = _users.find(uid);
    if (uit == _users.end())
        return room_ptr();
    uint64_t rid = uit->second;
    // 2. 通过房间ID获取房间信息
    auto rit = _rooms.find(rid);
    if (rit == _rooms.end())
        return room_ptr();
    return rit->second;
}
/*通过房间ID销毁房间*/
void remove_room(uint64_t rid)
{
    // 因为房间信息，是通过shared_ptr在_rooms中进行管理，因此只要将shared_ptr从_rooms中移除
    // 则shared_ptr计数器==0，外界没有对房间信息进行操作保存的情况下就会释放
    // 1. 通过房间ID，获取房间信息
    room_ptr rp = get_room_by_rid(rid);
    if (rp.get() == nullptr)
        return;

    // 2. 通过房间信息，获取房间中所有用户的ID
    uint64_t uid1 = rp->get_white_user();
    uint64_t uid2 = rp->get_black_user();

    // 3. 移除房间管理中的用户信息
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _users.erase(uid1);
        _users.erase(uid2);
        // 4. 移除房间管理信息
        _rooms.erase(rid);
    }
}

/*删除房间中指定用户，如果房间中没有用户了，则销毁房间，用户连接断开时被调用*/
void remove_room_user(uint64_t uid)
{
        room_ptr rp = get_room_by_uid(uid);
        if (rp.get() == nullptr)
            return;

        // 处理房间中玩家退出动作
        rp->handle_exit(uid);
        
        // 房间中没有玩家了，则销毁房间
        if (rp->player_count() == 0)
            remove_room(rp->id());
        return;
    }

private:
    uint64_t _next_rid; // 房间的id
    std::mutex _mutex;
    user_table *_tb_user;
    online_manager *_online_user;
    std::unordered_map<uint64_t, room_ptr> _rooms; // 房间信息管理
    std::unordered_map<uint64_t, uint64_t> _users; // 两个对战的用户管理
};

#endif
```

## 对战匹配

将所有玩家根据分数进行等级划分，进行不同等级的对战匹配。

匹配对战：

1. 将所有玩家，根据得分划分为三个档次

score < 2000;score >= 2000 && score < 3000;score >= 3000

1. 为三个不同档次创建三个不同的匹配队列
2. 如果有玩家要进行对战匹配，则根据玩家分数，将玩家的id，加入到指定的队列中
3. 当一个队列中元素数量`>=`2个，则表示有两个玩家要进行匹配，匹配成功
4. 出队队列中的前两个元素，为这个两个玩家创建房间
5. 向匹配成功的玩家，发送匹配响应：对战匹配成功，

![](https://cdn.nlark.com/yuque/0/2025/png/36043961/1755236510113-6b249462-a4d6-4036-a005-c04c66464f10.png)

设计一个阻塞队列：（目的是用于实现玩家匹配队列）

功能:

1. 入队数据
2. 出队数据
3. 移除指定的数据
4. 线程安全
5. 获取队列元素个数
6. 阻塞
7. 判断队列是否为空

### 类的设计

```cpp
#ifndef __MATCHER_HPP__
#define __MATCHER_HPP__

#include <list>
#include <mutex>
#include <condition_variable>

#include "util.hpp"
#include "online.hpp"
#include "db.hpp"
#include "room.hpp"

template <class T>
class match_queue
{
private:
/*用链表而不直接使用queue是因为我们有中间删除数据的需要*/
std::list<T> _list;
/*实现线程安全*/
std::mutex _mutex;
/*这个条件变量主要为了阻塞消费者，后边使用的时候：队列中元素个数<2则阻塞*/
std::condition_variable _cond;
public:
/*获取元素个数*/
int size()
{
}
/*判断是否为空*/
bool empty()
{
}
/*阻塞线程*/
void wait()
{
}
/*入队数据，并唤醒线程*/
void push(const T &data)
{
}
/*出队数据*/
bool pop(T &data)
{
}
/*移除指定的数据*/
void remove(T &data)
{
}
};

#endif
```

### 类的实现

```cpp
#ifndef __MATCHER_HPP__
#define __MATCHER_HPP__

#include <list>
#include <mutex>
#include <condition_variable>

#include "util.hpp"
#include "online.hpp"
#include "db.hpp"
#include "room.hpp"

template <class T>
class match_queue
{
private:
/*用链表而不直接使用queue是因为我们有中间删除数据的需要*/
std::list<T> _list;
/*实现线程安全*/
std::mutex _mutex;
/*这个条件变量主要为了阻塞消费者，后边使用的时候：队列中元素个数<2则阻塞*/
std::condition_variable _cond;

public:
/*获取元素个数*/
int size()
{
    std::unique_lock<std::mutex> lock(_mutex);
    return _list.size();
}
/*判断是否为空*/
bool empty()
{
    std::unique_lock<std::mutex> lock(_mutex);
    return _list.empty();
}
/*阻塞线程*/
void wait()
{
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock);
}
/*入队数据，并唤醒线程*/
void push(const T &data)
{
    std::unique_lock<std::mutex> lock(_mutex);
    _list.push_back(data);
    _cond.notify_all();
}
/*出队数据*/
bool pop(T &data)
{
    std::unique_lock<std::mutex> lock(_mutex);
    if (_list.empty() == true)
    {
        return false;
    }
    data = _list.front();
    _list.pop_front();
    return true;
}
/*移除指定的数据*/
void remove(T &data)
{
    std::unique_lock<std::mutex> lock(_mutex);
    _list.remove(data);
}
};

#endif
```

### 匹配管理

1. 三个不同档次的队列
2. 三个线程分别对三个队列中的玩家进行匹配
3. 房间管理模块的句柄
4. 在线用户管理模块的句柄
5. 数据管理模块-用户表的句柄

功能:

1. 添加用户到匹配队列
2. 从匹配队列移除用户
3. 线程入口函数

判断指定队列是否人数大于2

出队两个玩家

创建房间.将两个玩家添加到房间中

### 类的设计

```cpp
class matcher
{
private:
void handle_match(match_queue<uint64_t> &mq)
{
}
void th_normal_entry() { }
void th_high_entry() {  }
void th_super_entry() { }

public:
matcher(room_manager *rm, user_table *ut, online_manager *om)
{
}
bool add(uint64_t uid)
{
}
bool del(uint64_t uid)
{
}

private:
/*普通选手匹配队列*/
match_queue<uint64_t> _q_normal;
/*高手匹配队列*/
match_queue<uint64_t> _q_high;
/*大神匹配队列*/
match_queue<uint64_t> _q_super;

/*对应三个匹配队列的处理线程*/
std::thread _th_normal;
std::thread _th_high;
std::thread _th_super;

room_manager *_rm;
user_table *_ut;
online_manager *_om;
};
```

### 类的实现

```cpp
class matcher
{
private:
void handle_match(match_queue<uint64_t> &mq)
{
    for (;;)
    {
        // 1. 判断队列人数是否大于2，<2则阻塞等待
        while (mq.size() < 2)
            mq.wait();

        // 2. 走下来代表人数够了，出队两个玩家
        uint64_t uid1, uid2;
        bool ret = mq.pop(uid1);
        if (ret == false)
        {
            continue;
        }
        ret = mq.pop(uid2);
        if (ret == false)
        {
            this->add(uid1);
            continue;
        }
        // 3. 校验两个玩家是否在线，如果有人掉线，则要吧另一个人重新添加入队列
        wsserver_t::connection_ptr conn1 = _om->get_conn_from_hall(uid1);
        if (conn1.get() == nullptr)
        {
            this->add(uid2);
            continue;
        }
        wsserver_t::connection_ptr conn2 = _om->get_conn_from_hall(uid2);
        if (conn2.get() == nullptr)
        {
            this->add(uid1);
            continue;
        }
        // 4. 为两个玩家创建房间，并将玩家加入房间中
        room_ptr rp = _rm->create_room(uid1, uid2);
        if (rp.get() == nullptr)
        {
            this->add(uid1);
            this->add(uid2);
            continue;
        }

        // 5. 对两个玩家进行响应
        Json::Value resp;
        resp["optype"] = "match_success";
        resp["result"] = true;
        std::string body;
        json_util::serialize(resp, &body);
        conn1->send(body);
        conn2->send(body);
    }
}
void th_normal_entry() { return handle_match(_q_normal); }
void th_high_entry() { return handle_match(_q_high); }
void th_super_entry() { return handle_match(_q_super); }

public:
matcher(room_manager *rm, user_table *ut, online_manager *om)
: _rm(rm), _ut(ut), _om(om),
_th_normal(std::thread(&matcher::th_normal_entry, this)),
_th_high(std::thread(&matcher::th_high_entry, this)),
_th_super(std::thread(&matcher::th_super_entry, this))

{
    DLOG("游戏匹配模块初始化完毕....");
}
bool add(uint64_t uid)
{
    // 根据玩家的天梯分数，来判定玩家档次，添加到不同的匹配队列
    //  1. 根据用户ID，获取玩家信息
    Json::Value user;
    bool ret = _ut->select_by_id(uid, user);
    if (ret == false)
    {
        DLOG("获取玩家:%ld 信息失败！！", uid);
        return false;
    }
    int score = user["score"].asInt();
    // 2. 添加到指定的队列中
    if (score < 2000)
        _q_normal.push(uid);
    else if (score >= 2000 && score < 3000)
        _q_high.push(uid);
    else
        _q_super.push(uid);
    return true;
}
bool del(uint64_t uid)
{
    Json::Value user;
    bool ret = _ut->select_by_id(uid, user);
    if (ret == false)
    {
        DLOG("获取玩家:%ld 信息失败!", uid);
        return false;
    }
    int score = user["score"].asInt();
    // 2. 添加到指定的队列中
    if (score < 2000)
        _q_normal.remove(uid);
        else if (score >= 2000 && score < 3000)
            _q_high.remove(uid);
        else
            _q_super.remove(uid);
        return true;
    }

private:
    /*普通选手匹配队列*/
    match_queue<uint64_t> _q_normal;
    /*高手匹配队列*/
    match_queue<uint64_t> _q_high;
    /*大神匹配队列*/
    match_queue<uint64_t> _q_super;

    /*对应三个匹配队列的处理线程*/
    std::thread _th_normal;
    std::thread _th_high;
    std::thread _th_super;

    room_manager *_rm;
    user_table *_ut;
    online_manager *_om;
};
```

## 网络服务器模块

基于websocketpp库搭建websocket服务器，实现与客户端网络通信。

通过网络通信获取到客户端的请求，提供不同的业务处理

服务器的整合实现：

1. 网络通信接口的设计

收到一个什么格式的数据，代表了什么样的请求，应该给与什么样的业务处理及响应

1. 开始搭建服务器
   1. 搭建websocket服务器，实现网络通信
   2. 针对各种不同的请求进行不同的业务处理

http请求：

1. 客户端从服务器获取一个注册页面
2. 客户端给服务器发送一个注册请求(提交了用户名&密码）静态资源请求
3. 客户端从服务器获取一个登录页面
4. 客户端给服务器发送一个登录请求（提交了用户名&密码）
5. 客户端从服务器获取一个游戏大厅页面
6. 客户端给服务器发送了一个获取个人信息的请求（展示个人信息）

websocket：

1. 客户端给服务器发送了一个切换websocket协议通信的请求(建立游戏大厅长连接)
2. 客户端给服务器发送一个对战匹配请求
3. 客户端给服务器发送一个停止匹配请求
4. 对战匹配成功，客户端从服务器获取一个游戏房间页面
5. 客户端给服务器发送了一个切换websocket协议通信请求(建立游戏房间长连接)
6. 客户端给服务器发送一个下棋请求
7. 客户端给服务器发送一个聊天请求
8. 游戏结果，返回游戏大厅(客户端给服务器发送一个获取游戏大厅页面的请求)

### 静态资源请求

静态资源页面，在后台服务器上就是个html/css/js文件

静态资源请求的处理，其实就是将文件中的内容发送给客户端

1. 注册页面请求

请求: GET /register.html HTTP/1.1

响应：

HTTP/1.1 200 OK

Content-Length: xxx

Content-Type: text/html

register.html文件的内容数据

1. 登录页面请求

请求: GET /login.html HTTP/1.1

1. 大厅页面请求

请求: GET /game_hall.html HTTP/1.1

1. 房间页面请求

请求: GET /game_room.html HTTP/1.1

### 类的设计

```cpp
#ifndef __SERVER_HPP__
#define __SERVER_HPP__

#include "util.hpp"
#include "db.hpp"
#include "online.hpp"
#include "room.hpp"
#include "session.hpp"
#include "matcher.hpp"

#define WWWROOT "./wwwroot"

class gobang_server
{
private:
void http_callback(websocketpp::connection_hdl hdl)
{
}
void open_callback(websocketpp::connection_hdl hdl)
{
}
void close_callback(websocketpp::connection_hdl hdl)
{
}
void message_callback(websocketpp::connection_hdl hdl, wsserver_t::message_ptr msg)
{
}

public:
gobang_server(const std::string &host, const std::string &user,
const std::string &pass, const std::string &dbname,
uint16_t port = 3306, const std::string &wwwroot = WWWROOT)
: _web_root(wwwroot), _ut(host, user, pass, dbname, port),
_rm(&_ut, &_om), _sm(&_wssrv), _mm(&_rm, &_ut, &_om)
{
    // 初始化websocket服务器
    _wssrv.set_access_channels(websocketpp::log::alevel::none);
    _wssrv.init_asio();
    _wssrv.set_reuse_addr(true);
    _wssrv.set_http_handler(std::bind(&gobang_server::http_callback, this, std::placeholders::_1));
    _wssrv.set_open_handler(std::bind(&gobang_server::open_callback, this, std::placeholders::_1));
    _wssrv.set_close_handler(std::bind(&gobang_server::close_callback, this, std::placeholders::_1));
    _wssrv.set_message_handler(std::bind(&gobang_server::message_callback, this, std::placeholders::_1, std::placeholders::_2));
}

// 启动服务器
void run(int port)
{
    _wssrv.listen(port);
    _wssrv.start_accept();
    _wssrv.run();
}

private:
std::string _web_root; // 静态资源根目录
wsserver_t _wssrv;     // websocket服务器
user_table _ut;        // 用户表
online_manager _om;    // 在线管理器
room_manager _rm;      // 房间管理器
session_manager _sm;   // 会话管理器
matcher _mm;           // 匹配器
};

#endif
```

## 准备静态资源

[下载链接](https://shilinnull.feishu.cn/wiki/CF97wvmTUiB80rkLZ6XcSHMYnmb#share-SwRVdxdWgo64qdxD1omcWSewnyh)

### 类的实现

```cpp
#ifndef __SERVER_HPP__
#define __SERVER_HPP__

#include "util.hpp"
#include "db.hpp"
#include "online.hpp"
#include "room.hpp"
#include "session.hpp"
#include "matcher.hpp"

#define WWWROOT "./wwwroot/"

class gobang_server
{
private:
void http_resp(wsserver_t::connection_ptr &conn, bool result,
websocketpp::http::status_code::value code, const std::string &reason)
{
    Json::Value resp_json;
    resp_json["result"] = result;
    resp_json["reason"] = reason;
    std::string resp_body;
    json_util::serialize(resp_json, &resp_body);
    conn->set_status(code);
    conn->set_body(resp_body);
    conn->append_header("Content-type", "application/json");
    return;
}

void ws_resp(wsserver_t::connection_ptr &conn, bool result,
const std::string &optype, const std::string &reason)
{
    Json::Value resp_json;
    resp_json["optype"] = optype;
    resp_json["reason"] = reason;
    resp_json["result"] = result;

    std::string body;
    json_util::serialize(resp_json, &body);
    conn->send(body);
}

bool get_cookie_val(const std::string &cookie_str, const std::string &key, std::string &val)
{
    // 1. 以 ; 作为间隔，对字符串进行分割，得到各个单个的cookie信息
    const std::string sep = "; ";
    std::vector<std::string> cookie_arr;
    string_util::split(cookie_str, sep, cookie_arr);
    for (auto str : cookie_arr)
        {
            // 2. 对单个cookie字符串，以 = 为间隔进行分割，得到key和val
            std::vector<std::string> t;
            string_util::split(str, "=", t);
            if (t.size() != 2)
                continue;
            if (t[0] == key)
            {
                val = t[1];
                return true;
            }
        }
    return false;
}

session_ptr get_session_by_cookie(wsserver_t::connection_ptr &conn)
{
    // 1. 获取请求信息中的Cookie，从Cookie中获取ssid
    std::string cookie_str = conn->get_request_header("Cookie");
    if (cookie_str.empty())
    {
        // 如果没有cookie，返回错误：没有cookie信息，让客户端重新登录
        ws_resp(conn, false, "hall_ready", "没有找到cookie信息, 需要重新登录");
        return session_ptr();
    }
    // 1.5 从cookie中取出ssid
    std::string ssid_str;
    bool ret = get_cookie_val(cookie_str, "SSID", ssid_str);
    if (!ret)
    {
        ws_resp(conn, false, "hall_ready", "没有找到SSID信息, 需要重新登录");
        return session_ptr();
    }
    // 2. 在session管理中查找对应的会话信息
    session_ptr ssp = _sm.get_session_by_ssid(std::stol(ssid_str));
    if (ssp.get() == nullptr)
    {
        ws_resp(conn, false, "hall_ready", "没有找到session信息, 需要重新登录");
        return session_ptr();
    }
    return ssp;
}

void open_game_hall(wsserver_t::connection_ptr conn)
{
    // 1. 登录验证--判断当前客户端是否已经成功登录
    session_ptr ssp = get_session_by_cookie(conn);
    if (ssp.get() == nullptr)
        return;

        // 2. 判断当前客户端是否是重复登录
        if (_om.is_in_game_hall(ssp->get_user()) || _om.is_in_game_room(ssp->get_user()))
            return ws_resp(conn, false, "hall_ready", "玩家重复登录！");
        // 3. 将当前客户端以及连接加入到游戏大厅
        _om.enter_game_hall(ssp->get_user(), conn);
        // 4. 给客户端响应游戏大厅连接建立成功
        ws_resp(conn, true, "hall_ready", "大厅连接建立成功");
        // 5. 将session设置为永久存在
        _sm.set_session_expire_time(ssp->ssid(), SESSION_FOREVER);
    }

    void open_game_room(wsserver_t::connection_ptr conn)
    {
        // 1. 获取当前客户端的session
        session_ptr ssp = get_session_by_cookie(conn);
        if (ssp.get() == nullptr)
            return;
        // 2. 当前用户是否已经在在线用户管理的游戏房间或者游戏大厅中---在线用户管理
        if (_om.is_in_game_hall(ssp->get_user()) || _om.is_in_game_room(ssp->get_user()))
            return ws_resp(conn, false, "room_ready", "玩家重复登录！");
        // 3. 判断当前用户是否已经创建好了房间 --- 房间管理
        room_ptr rp = _rm.get_room_by_uid(ssp->get_user());
        if (rp.get() == nullptr)
        {
            return ws_resp(conn, false, "room_ready", "没有找到玩家的房间信息");
        }

        // 4. 将当前用户添加到在线用户管理的游戏房间中
        _om.enter_game_room(ssp->get_user(), conn);
        // 5. 将session重新设置为永久存在
        _sm.set_session_expire_time(ssp->ssid(), SESSION_FOREVER);
        // 6. 回复房间准备完毕
        Json::Value resp_json;
        resp_json["optype"] = "room_ready";
        resp_json["result"] = true;
        resp_json["room_id"] = (Json::UInt64)rp->id();
        resp_json["uid"] = (Json::UInt64)ssp->get_user();
        resp_json["white_id"] = (Json::UInt64)rp->get_white_user();
        resp_json["black_id"] = (Json::UInt64)rp->get_black_user();
        std::string body;
        json_util::serialize(resp_json, &body);
        conn->send(body);
        return;
    }

    void close_game_hall(wsserver_t::connection_ptr &conn)
    {
        // 1. 登录验证--判断当前客户端是否已经成功登录
        session_ptr ssp = get_session_by_cookie(conn);
        if (ssp.get() == nullptr)
            return;
        // 2. 将玩家从游戏大厅中移除
        _om.exit_game_hall(ssp->get_user());
        // 3. 将session恢复生命周期的管理，设置定时销毁
        _sm.set_session_expire_time(ssp->ssid(), SESSION_TIMEOUT);
    }

    void close_game_room(wsserver_t::connection_ptr &conn)
    {
        // 获取会话信息，识别客户端
        session_ptr ssp = get_session_by_cookie(conn);
        if (ssp.get() == nullptr)
            return;
        // 1. 将玩家从在线用户管理中移除
        _om.exit_game_room(ssp->get_user());
        // 2. 将session回复生命周期的管理，设置定时销毁
        _sm.set_session_expire_time(ssp->ssid(), SESSION_TIMEOUT);
        // 3. 将玩家从游戏房间中移除，房间中所有用户退出了就会销毁房间
        _rm.remove_room_user(ssp->get_user());
    }

    void msg_game_hall(wsserver_t::connection_ptr &conn, wsserver_t::message_ptr &msg)
    {
        // 1. 身份验证，当前客户端到底是哪个玩家
        session_ptr ssp = get_session_by_cookie(conn);
        if (ssp.get() == nullptr)
            return;

        // 2. 获取请求信息
        std::string req_body = msg->get_payload();
        Json::Value req_json;
        bool ret = json_util::unserialize(req_body, &req_json);
        if (!ret)
            return ws_resp(conn, false, "", "请求信息解析失败");
        // 3. 对于请求进行处理
        if (!req_json["optype"].isNull() && req_json["optype"].asString() == "match_start")
        {
            //  开始对战匹配：通过匹配模块，将用户添加到匹配队列中
            _mm.add(ssp->get_user());
            return ws_resp(conn, true, "match_start", "");
        }
        else if (!req_json["optype"].isNull() && req_json["optype"].asString() == "match_stop")
        {
            //  停止对战匹配：通过匹配模块，将用户从匹配队列中移除
            _mm.del(ssp->get_user());
            return ws_resp(conn, true, "match_stop", "");
        }
        return ws_resp(conn, false, "Unknow", "请求类型未知");
    }

    void msg_game_room(wsserver_t::connection_ptr &conn, wsserver_t::message_ptr &msg)
    {
        // 1. 获取客户端session，识别客户端身份
        session_ptr ssp = get_session_by_cookie(conn);
        if (ssp.get() == nullptr)
        {
            DLOG("房间-没有找到会话信息");
            return;
        }

        // 2. 获取客户端房间信息
        room_ptr rp = _rm.get_room_by_uid(ssp->get_user());
        if (rp.get() == nullptr)
        {
            DLOG("房间-没有找到玩家房间信息");
            return ws_resp(conn, false, "Unknow", "没有找到玩家的房间信息");
        }
        // 3. 对消息进行反序列化
        Json::Value req_json;
        std::string req_body = msg->get_payload();
        bool ret = json_util::unserialize(req_body, &req_json);
        if (!ret)
        {
            DLOG("房间-反序列化请求失败");
            return ws_resp(conn, false, "unknow", "请求解析失败");
        }
        DLOG("房间：收到房间请求，开始处理....");
        // 4. 通过房间模块进行消息请求的处理
        return rp->handle_request(req_json);
    }

private:
    // 静态资源请求
    void file_handler(wsserver_t::connection_ptr &conn)
    {
        // 1. 获取到请求uri-资源路径，了解客户端请求的页面文件名称
        websocketpp::http::parser::request req = conn->get_request();
        std::string uri = req.get_uri();
        // 2. 组合出文件的实际路径   相对根目录 + uri
        std::string realpath = _web_root + uri;
        // 3. 如果请求的是个目录，增加一个后缀  login.html,    /  ->  /login.html
        if (realpath.back() == '/')
        {
            realpath += "login.html";
        }
        // 4. 读取文件内容
        std::string body;
        bool ret = file_util::read(realpath, body);
        if (!ret)
        {
            body += "<html>";
            body += "<head>";
            body += "<meta charset='UTF-8'/>";
            body += "</head>";
            body += "<body>";
            body += "<h1> Not Found </h1>";
            body += "</body>";
            conn->set_status(websocketpp::http::status_code::not_found);
            conn->set_body(body);
            return;
        }
        conn->set_status(websocketpp::http::status_code::ok);
        conn->set_body(body);
    }

    // 注册请求
    void reg(wsserver_t::connection_ptr &conn)
    {
        websocketpp::http::parser::request req = conn->get_request();
        // 1. 获取到请求正文
        std::string req_body = conn->get_request_body();

        // 2. 对正文进行json反序列化，得到用户名和密码
        Json::Value login_info;
        bool ret = json_util::unserialize(req_body, &login_info);
        if (!ret)
        {
            DLOG("反序列化注册信息失败");
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "请求的正文格式错误");
        }

        // 3. 进行数据库的用户新增操作
        if (login_info["username"].isNull() || login_info["password"].isNull())
        {
            DLOG("用户名密码不完整");
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "请输入用户名/密码");
        }

        ret = _ut.insert(login_info);
        if (!ret)
        {
            DLOG("向数据库插入数据失败");
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "用户名已经被占用");
        }
        return http_resp(conn, true, websocketpp::http::status_code::ok, "注册用户成功");
    }

    // 登录请求
    void login(wsserver_t::connection_ptr &conn)
    {
        // 1. 获取请求正文，并进行json反序列化，得到用户名和密码
        std::string req_body = conn->get_request_body();
        Json::Value login_info;
        bool ret = json_util::unserialize(req_body, &login_info);
        if (ret == false)
        {
            DLOG("反序列化登录信息失败");
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "请求的正文格式错误");
        }
        // 2. 校验正文完整性，进行数据库的用户信息验证
        if (login_info["username"].isNull() || login_info["password"].isNull())
        {
            DLOG("用户名密码不完整");
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "请输入用户名/密码");
        }
        ret = _ut.login(login_info);
        if (ret == false)
        {
            //  1. 如果验证失败，则返回400
            DLOG("用户名密码错误");
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "用户名密码错误");
        }
        // 3. 如果验证成功，给客户端创建session
        uint64_t uid = login_info["id"].asUInt64();
        session_ptr ssp = _sm.create_session(uid, LOGIN);
        if (ssp.get() == nullptr)
        {
            DLOG("创建会话失败");
            return http_resp(conn, false, websocketpp::http::status_code::internal_server_error, "创建会话失败");
        }
        _sm.set_session_expire_time(ssp->ssid(), SESSION_TIMEOUT);
        // 4. 设置响应头部：Set-Cookie,将sessionid通过cookie返回
        std::string cookie_ssid = "SSID=" + std::to_string(ssp->ssid());
        conn->append_header("Set-Cookie", cookie_ssid);
        return http_resp(conn, true, websocketpp::http::status_code::ok, "登录成功");
    }

    // 用户信息请求
    void info(wsserver_t::connection_ptr &conn)
    {
        Json::Value err_resp;
        // 1. 获取请求信息中的Cookie，从Cookie中获取ssid

        std::string cookie_str = conn->get_request_header("Cookie");
        if (cookie_str.empty())
        {
            // 如果没有cookie，返回错误：没有cookie信息，让客户端重新登录
            return http_resp(conn, true, websocketpp::http::status_code::bad_request, "找不到cookie信息，请重新登录");
        }

        // 1.5. 从cookie中取出ssid
        std::string ssid_str;
        bool ret = get_cookie_val(cookie_str, "SSID", ssid_str);
        if (!ret)
        {
            // cookie中没有ssid，返回错误：没有ssid信息，让客户端重新登录
            return http_resp(conn, true, websocketpp::http::status_code::bad_request, "找不到ssid信息，请重新登录");
        }
        // 2. 在session管理中查找对应的会话信息
        session_ptr ssp = _sm.get_session_by_ssid(std::stol(ssid_str));
        if (ssp.get() == nullptr)
        {
            // 没有找到session，则认为登录已经过期，需要重新登录
            return http_resp(conn, true, websocketpp::http::status_code::bad_request, "登录过期，请重新登录");
        }
        // 3. 从数据库中取出用户信息，进行序列化发送给客户端
        uint64_t uid = ssp->get_user();
        Json::Value user_info;
        ret = _ut.select_by_id(uid, user_info);
        if (ret == false)
        {
            // 获取用户信息失败，返回错误：找不到用户信息
            return http_resp(conn, true, websocketpp::http::status_code::bad_request, "找不到用户信息，请重新登录");
        }
        std::string body;
        json_util::serialize(user_info, &body);
        conn->set_body(body);
        conn->append_header("Content-Type", "application/json");
        conn->set_status(websocketpp::http::status_code::ok);
        // 4. 刷新session的过期时间
        _sm.set_session_expire_time(ssp->ssid(), SESSION_TIMEOUT);
    }

    void http_callback(websocketpp::connection_hdl hdl)
    {
        wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hdl); // 获取连接对象
        websocketpp::http::parser::request req = conn->get_request();   // 获取请求对象
        std::string method = req.get_method();                          // 获取请求方法
        std::string uri = req.get_uri();                                // 获取请求uri
        if (method == "POST" && uri == "/reg")
        {
            return reg(conn);
        }
        else if (method == "POST" && uri == "/login")
        {
            return login(conn);
        }
        else if (method == "GET" && uri == "/info")
        {
            return info(conn);
        }
        else
        {
            return file_handler(conn);
        }
    }

    void open_callback(websocketpp::connection_hdl hdl)
    {
        // websocket长连接建立成功之后的处理函数
        wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hdl);
        websocketpp::http::parser::request req = conn->get_request();
        std::string uri = req.get_uri();
        if (uri == "/hall")
        {
            // 建立了游戏大厅的长连接
            return open_game_hall(conn);
        }
        else if (uri == "/room")
        {
            // 建立了游戏房间的长连接
            return open_game_room(conn);
        }
    }

    void close_callback(websocketpp::connection_hdl hdl)
    {
        wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hdl);
        websocketpp::http::parser::request req = conn->get_request();
        std::string uri = req.get_uri();
        if (uri == "/hall")
        {
            // 建立了游戏大厅的长连接
            return close_game_hall(conn);
        }
        else if (uri == "/room")
        {
            // 建立了游戏房间的长连接
            return close_game_room(conn);
        }
    }

    void message_callback(websocketpp::connection_hdl hdl, wsserver_t::message_ptr msg)
    {
        // websocket长连接通信处理
        wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hdl);
        websocketpp::http::parser::request req = conn->get_request();
        std::string uri = req.get_uri();
        if (uri == "/hall")
        {
            // 建立了游戏大厅的长连接
            return msg_game_hall(conn, msg);
        }
        else if (uri == "/room")
        {
            // 建立了游戏房间的长连接
            return msg_game_room(conn, msg);
        }
    }

public:
    gobang_server(const std::string &host, const std::string &user,
                  const std::string &pass, const std::string &dbname,
                  uint16_t port = 3306, const std::string &wwwroot = WWWROOT)
        : _web_root(wwwroot), _ut(host, user, pass, dbname, port),
          _rm(&_ut, &_om), _sm(&_wssrv), _mm(&_rm, &_ut, &_om)
    {
        // 初始化websocket服务器
        _wssrv.set_access_channels(websocketpp::log::alevel::none);
        _wssrv.init_asio();
        _wssrv.set_reuse_addr(true);
        _wssrv.set_http_handler(std::bind(&gobang_server::http_callback, this, std::placeholders::_1));
        _wssrv.set_open_handler(std::bind(&gobang_server::open_callback, this, std::placeholders::_1));
        _wssrv.set_close_handler(std::bind(&gobang_server::close_callback, this, std::placeholders::_1));
        _wssrv.set_message_handler(std::bind(&gobang_server::message_callback, this, std::placeholders::_1, std::placeholders::_2));
    }

    // 启动服务器
    void run(int port)
    {
        _wssrv.listen(port);
        _wssrv.start_accept();
        _wssrv.run();
    }

private:
    std::string _web_root; // 静态资源根目录
    wsserver_t _wssrv;     // websocket服务器
    user_table _ut;        // 用户表
    online_manager _om;    // 在线管理器
    room_manager _rm;      // 房间管理器
    session_manager _sm;   // 会话管理器
    matcher _mm;           // 匹配器
};

#endif
```

# 客户端开发

## 登录页面：login.html

```html
<!DOCTYPE html>
<html lang="en">

  <head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>登录</title>

    <link rel="stylesheet" href="./css/common.css">
    <link rel="stylesheet" href="./css/login.css">
  </head>

  <body>
    <div class="nav">
      网络五子棋对战游戏
    </div>
    <div class="login-container">
      <!-- 登录界面的对话框 -->
      <div class="login-dialog">
        <!-- 提示信息 -->
        <h3>登录</h3>
        <!-- 这个表示一行 -->
        <div class="row">
          <span>用户名</span>
          <input type="text" id="user_name">
        </div>
        <!-- 这是另一行 -->
        <div class="row">
          <span>密码</span>
          <input type="password" id="password">
        </div>
        <!-- 提交按钮 -->
        <div class="row">
          <button id="submit" onclick="login()">提交</button>
        </div>
        <!-- 注册跳转按钮 -->
        <div class="row">
          <button onclick="window.location.assign('/register.html')">没有账号？去注册</button>
        </div>

      </div>

      <script src="./js/jquery.min.js"></script>
      <script>
        //1. 给按钮添加点击事件，调用登录请求函数
        //2. 封装登录请求函数
        function login() {
          //  1. 获取输入框中的用户名和密码，并组织json对象
          var login_info = {
            username: document.getElementById("user_name").value,
            password: document.getElementById("password").value
          };
          //  2. 通过ajax向后台发送登录验证请求
          $.ajax({
            url: "/login",
            type: "post",
            data: JSON.stringify(login_info),
            success: function (result) {
              //  3. 如果验证通过，则跳转游戏大厅页面
              alert("登录成功");
              window.location.assign("/game_hall.html");
            },
            error: function (xhr) {
              //  4. 如果验证失败，则提示错误信息，并清空输入框
              alert(JSON.stringify(xhr));
              document.getElementById("user_name").value = "";
              document.getElementById("password").value = "";
            }
          })
        }
      </script>
  </body>

</html>
```

## 注册页面：register.html

```html
<!DOCTYPE html>
<html lang="en">

  <head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>登录</title>

    <link rel="stylesheet" href="./css/common.css">
    <link rel="stylesheet" href="./css/login.css">
  </head>

  <body>
    <div class="nav">
      网络五子棋对战游戏
    </div>
    <div class="login-container">
      <!-- 登录界面的对话框 -->
      <div class="login-dialog">
        <!-- 提示信息 -->
        <h3>登录</h3>
        <!-- 这个表示一行 -->
        <div class="row">
          <span>用户名</span>
          <input type="text" id="user_name">
        </div>
        <!-- 这是另一行 -->
        <div class="row">
          <span>密码</span>
          <input type="password" id="password">
        </div>
        <!-- 提交按钮 -->
        <div class="row">
          <button id="submit" onclick="login()">提交</button>
        </div>
        <!-- 注册跳转按钮 -->
        <div class="row">
          <button onclick="window.location.assign('/register.html')">没有账号？去注册</button>
        </div>

      </div>

      <script src="./js/jquery.min.js"></script>
      <script>
        //1. 给按钮添加点击事件，调用登录请求函数
        //2. 封装登录请求函数
        function login() {
          //  1. 获取输入框中的用户名和密码，并组织json对象
          var login_info = {
            username: document.getElementById("user_name").value,
            password: document.getElementById("password").value
          };
          //  2. 通过ajax向后台发送登录验证请求
          $.ajax({
            url: "/login",
            type: "post",
            data: JSON.stringify(login_info),
            success: function (result) {
              //  3. 如果验证通过，则跳转游戏大厅页面
              alert("登录成功");
              window.location.assign("/game_hall.html");
            },
            error: function (xhr) {
              //  4. 如果验证失败，则提示错误信息，并清空输入框
              alert(JSON.stringify(xhr));
              document.getElementById("user_name").value = "";
              document.getElementById("password").value = "";
            }
          })
        }
      </script>
  </body>

</html>
```

## 游戏大厅：game_hall.html

```html
<!DOCTYPE html>
<html lang="en">

  <head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>游戏大厅</title>
    <link rel="stylesheet" href="./css/common.css">
    <link rel="stylesheet" href="./css/game_hall.css">
  </head>

  <body>
    <div class="nav">网络五子棋对战游戏</div>
    <!-- 整个页面的容器元素 -->
    <div class="container">
      <!-- 这个 div 在 container 中是处于垂直水平居中这样的位置的 -->
      <div>
        <!-- 展示用户信息 -->
        <div id="screen">
        </div>
        <!-- 匹配按钮 -->
        <div id="match-button">开始匹配</div>
      </div>
    </div>

    <script src="./js/jquery.min.js"></script>
    <script>
      var ws_url = "ws://" + location.host + "/hall";
      var ws_hdl = null;

      window.onbeforeunload = function () {
        ws_hdl.close();
      }
      //按钮有两个状态：没有进行匹配的状态，正在匹配中的状态
      var button_flag = "stop";
      //点击按钮的事件处理：
      var be = document.getElementById("match-button");
      be.onclick = function () {
        if (button_flag == "stop") {
          //1. 没有进行匹配的状态下点击按钮，发送对战匹配请求
          var req_json = {
            optype: "match_start"
          }
          ws_hdl.send(JSON.stringify(req_json));
        } else {
          //2. 正在匹配中的状态下点击按钮，发送停止对战匹配请求
          var req_json = {
            optype: "match_stop"
          }
          ws_hdl.send(JSON.stringify(req_json));
        }
      }
      function get_user_info() {
        $.ajax({
          url: "/info",
          type: "get",
          success: function (res) {
            var info_html = "<p>" + "用户：" + res.username + " 积分：" + res.score +
              "</br>" + "比赛场次：" + res.total_count + " 获胜场次：" + res.win_count + "</p>";
            var screen_div = document.getElementById("screen");
            screen_div.innerHTML = info_html;

            ws_hdl = new WebSocket(ws_url);
            ws_hdl.onopen = ws_onopen;
            ws_hdl.onclose = ws_onclose;
            ws_hdl.onerror = ws_onerror;
            ws_hdl.onmessage = ws_onmessage;
          },
          error: function (xhr) {
            // alert(JSON.stringify(xhr));
            alert("错误请求，重新登录");
            location.replace("/login.html");
          }
        })
      }
      function ws_onopen() {
        console.log("websocket onopen");
      }
      function ws_onclose() {
        console.log("websocket onopen");
      }
      function ws_onerror() {
        console.log("websocket onopen");
      }
      function ws_onmessage(evt) {
        var rsp_json = JSON.parse(evt.data);
        if (rsp_json.result == false) {
          // alert(evt.data);
          alert("错误请求，重新登录");
          location.replace("/login.html");
          return;
            }
            if (rsp_json["optype"] == "hall_ready") {
                alert("游戏大厅连接建立成功！");
            } else if (rsp_json["optype"] == "match_success") {
                //对战匹配成功
                alert("对战匹配成功，进入游戏房间！");
                location.replace("/game_room.html");
            } else if (rsp_json["optype"] == "match_start") {
                console.log("玩家已经加入匹配队列");
                button_flag = "start";
                be.innerHTML = "匹配中....点击按钮停止匹配!";
                return;
            } else if (rsp_json["optype"] == "match_stop") {
                console.log("玩家已经移除匹配队列");
                button_flag = "stop";
                be.innerHTML = "开始匹配";
                return;
            } else {
                // alert(evt.data);
                alert("错误请求，重新登录");
                location.replace("/login.html");
                return;
            }
        }
        get_user_info();

    </script>
</body>

</html>
```

## 游戏房间：game_room.html

```html
<!DOCTYPE html>
<html lang="en">

  <head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>游戏房间</title>
    <link rel="stylesheet" href="css/common.css">
    <link rel="stylesheet" href="css/game_room.css">
  </head>

  <body>
    <div class="nav">网络五子棋对战游戏</div>
    <div class="container">
      <div id="chess_area">
        <!-- 棋盘区域, 需要基于 canvas 进行实现 -->
        <canvas id="chess" width="450px" height="450px"></canvas>
        <!-- 显示区域 -->
        <div id="screen"> 等待玩家连接中... </div>
      </div>
      <div id="chat_area" width="400px" height="300px">
        <div id="chat_show">
          <p id="self_msg">你好！</p></br>
          <p id="peer_msg">你好！</p></br>
        </div>
        <div id="msg_show">
          <input type="text" id="chat_input">
          <button id="chat_button">发送</button>
        </div>
      </div>
    </div>
    <script>
      let chessBoard = [];
      let BOARD_ROW_AND_COL = 15;
      let chess = document.getElementById('chess');
      let context = chess.getContext('2d');//获取chess控件的2d画布

      var ws_url = "ws://" + location.host + "/room";
      var ws_hdl = new WebSocket(ws_url);

      var room_info = null;//用于保存房间信息 
      var is_me;

      function initGame() {
        initBoard();
        context.strokeStyle = "#BFBFBF";
        // 背景图片
        let logo = new Image();
        logo.src = "image/sky.jpeg";
        logo.onload = function () {
          // 绘制图片
          context.drawImage(logo, 0, 0, 450, 450);
          // 绘制棋盘
          drawChessBoard();
        }
      }
      function initBoard() {
        for (let i = 0; i < BOARD_ROW_AND_COL; i++) {
          chessBoard[i] = [];
          for (let j = 0; j < BOARD_ROW_AND_COL; j++) {
            chessBoard[i][j] = 0;
          }
        }
      }
      // 绘制棋盘网格线
      function drawChessBoard() {
        for (let i = 0; i < BOARD_ROW_AND_COL; i++) {
          context.moveTo(15 + i * 30, 15);
          context.lineTo(15 + i * 30, 430); //横向的线条
          context.stroke();
          context.moveTo(15, 15 + i * 30);
          context.lineTo(435, 15 + i * 30); //纵向的线条
          context.stroke();
        }
      }
      //绘制棋子
      function oneStep(i, j, isWhite) {
        if (i < 0 || j < 0) return;
        context.beginPath();
        context.arc(15 + i * 30, 15 + j * 30, 13, 0, 2 * Math.PI);
        context.closePath();
        var gradient = context.createRadialGradient(15 + i * 30 + 2, 15 + j * 30 - 2, 13, 15 + i * 30 + 2, 15 + j * 30 - 2, 0);
        // 区分黑白子
        if (!isWhite) {
          gradient.addColorStop(0, "#0A0A0A");
          gradient.addColorStop(1, "#636766");
        } else {
          gradient.addColorStop(0, "#D1D1D1");
                gradient.addColorStop(1, "#F9F9F9");
            }
            context.fillStyle = gradient;
            context.fill();
        }
        //棋盘区域的点击事件
        chess.onclick = function (e) {
            //  1. 获取下棋位置，判断当前下棋操作是否正常
            //      1. 当前是否轮到自己走棋了
            //      2. 当前位置是否已经被占用
            //  2. 向服务器发送走棋请求
            if (!is_me) {
                alert("等待对方走棋....");
                return;
            }
            let x = e.offsetX;
            let y = e.offsetY;
            // 注意, 横坐标是列, 纵坐标是行
            // 这里是为了让点击操作能够对应到网格线上
            let col = Math.floor(x / 30);
            let row = Math.floor(y / 30);
            if (chessBoard[row][col] != 0) {
                alert("当前位置已有棋子！");
                return;
            }
            //向服务器发送走棋请求，收到响应后，再绘制棋子
            send_chess(row, col);
        }
        function send_chess(r, c) {
            var chess_info = {
                optype: "put_chess",
                room_id: room_info.room_id,
                uid: room_info.uid,
                row: r,
                col: c
            };
            ws_hdl.send(JSON.stringify(chess_info));
            console.log("click:" + JSON.stringify(chess_info));
        }

        window.onbeforeunload = function () {
            ws_hdl.close();
        }
        ws_hdl.onopen = function () {
            console.log("房间长连接建立成功");
        }
        ws_hdl.onclose = function () {
            console.log("房间长连接断开");
        }
        ws_hdl.onerror = function () {
            console.log("房间长连接出错");
        }

        function set_screen(me) {
            var screen_div = document.getElementById("screen");
            if (me) {
                screen_div.innerHTML = "轮到己方走棋...";
            } else {
                screen_div.innerHTML = "轮到对方走棋...";
            }
        }
        ws_hdl.onmessage = function (evt) {
            var info = JSON.parse(evt.data);
            console.log(JSON.stringify(info));
            if (info.optype == "room_ready") {
                room_info = info;
                is_me = room_info.uid == room_info.white_id ? true : false;
                set_screen(is_me);
                initGame();

            } else if (info.optype == "put_chess") {
                console.log("put_chess" + evt.data);
                if (info.result == false) {
                    alert(info.reason);
                    return;
                }
                // 通过用户id来确定
                is_me = info.uid == room_info.uid ? false : true;
                set_screen(is_me);

                console.log("is_me " + is_me);
                console.log("info.uid " + info.uid);
                console.log("room_info.uid " + room_info.uid);

                // 检查是否是黑白棋
                isWhite = info.uid == room_info.white_id ? true : false;
                //绘制棋子
                if (info.row != -1 && info.col != -1) {
                    oneStep(info.col, info.row, isWhite);
                    //设置棋盘信息
                    chessBoard[info.row][info.col] = 1;
                }
                //是否有胜利者
                if (info.winner == 0) {
                    return;
                }
                var screen_div = document.getElementById("screen");
                if (room_info.uid == info.winner) {
                    screen_div.innerHTML = info.reason;
                } else {
                    screen_div.innerHTML = "你输了";
                }

                var chess_area_div = document.getElementById("chess_area");
                var button_div = document.createElement("div");
                button_div.innerHTML = "返回大厅";
                // 添加样式
                button_div.style.cssText = `
                    display: inline-block;
                    padding: 10px 20px;
                    background: linear-gradient(135deg, #6e8efb, #a777e3);
                    color: white;
                    border-radius: 25px;
                    font-weight: bold;
                    text-align: center;
                    cursor: pointer;
                    box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
                    transition: all 0.3s ease;
                    margin: 10px;
                    border: none;
                    outline: none;
                `;

                // 添加悬停效果
                button_div.addEventListener('mouseenter', function () {
                    this.style.transform = 'translateY(-2px)';
                    this.style.boxShadow = '0 6px 8px rgba(0, 0, 0, 0.15)';
                });

                button_div.addEventListener('mouseleave', function () {
                    this.style.transform = 'translateY(0)';
                    this.style.boxShadow = '0 4px 6px rgba(0, 0, 0, 0.1)';
                });

                // 添加点击效果
                button_div.addEventListener('mousedown', function () {
                    this.style.transform = 'translateY(1px)';
                    this.style.boxShadow = '0 2px 4px rgba(0, 0, 0, 0.1)';
                });

                chess_area_div.appendChild(button_div);
                button_div.onclick = function () {
                    ws_hdl.close();
                    location.replace("/game_hall.html");
                }
                chess_area_div.appendChild(button_div);
            } else if (info.optype == "chat") {
                //收到一条消息，判断result，如果为true则渲染一条消息到显示框中
                if (info.result == false) {
                    alert(info.reason);
                    return;
                }
                var msg_div = document.createElement("p");
                msg_div.innerHTML = info.message;
                if (info.uid == room_info.uid) {
                    msg_div.setAttribute("id", "self_msg");
                } else {
                    msg_div.setAttribute("id", "peer_msg");
                }
                var br_div = document.createElement("br");
                var msg_show_div = document.getElementById("chat_show");
                msg_show_div.appendChild(msg_div);
                msg_show_div.appendChild(br_div);
                document.getElementById("chat_input").value = "";
            }
        }
        //3. 聊天动作
        //  1. 捕捉聊天输入框消息
        //  2. 给发送按钮添加点击事件，点击俺就的时候，获取到输入框消息，发送给服务器
        var cb_div = document.getElementById("chat_button");
        cb_div.onclick = function () {
            var send_msg = {
                optype: "chat",
                room_id: room_info.room_id,
                uid: room_info.uid,
                message: document.getElementById("chat_input").value
            };
            ws_hdl.send(JSON.stringify(send_msg));
        }
        document.getElementById("chat_input").addEventListener("keydown", function (event) {
            // 检查是否按下了回车键
            if (event.key === "Enter") {
                // 调用目标函数
                var send_msg = {
                    optype: "chat",
                    room_id: room_info.room_id,
                    uid: room_info.uid,
                    message: document.getElementById("chat_input").value
                };
                ws_hdl.send(JSON.stringify(send_msg));
            }
        });

    </script>
</body>

</html>
```

