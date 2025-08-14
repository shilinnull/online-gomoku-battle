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