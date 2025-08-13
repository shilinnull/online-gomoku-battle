#include "db.hpp"
#include "online.hpp"
#include "session.hpp"
#include "matcher.hpp"

#define HOST "127.0.0.1"
#define PORT 3306
#define USER "root"
#define PASS "123456"
#define DBNAME "gobang"

void mysql_test()
{
    MYSQL *mysql = mysql_util::mysql_create(HOST, USER, PASS, DBNAME, PORT);
    const char *sql = "insert stu values(null, '小黑', 18, 53, 68, 87);";
    bool ret = mysql_util::mysql_exec(mysql, sql);
    if (ret == false)
    {
        return;
    }
    mysql_util::mysql_destroy(mysql);
}
void json_test()
{
    Json::Value root;
    std::string body;
    root["姓名"] = "小明";
    root["年龄"] = 18;
    root["成绩"].append(98);
    root["成绩"].append(88.5);
    root["成绩"].append(78.5);
    json_util::serialize(root, &body);
    DLOG("%s", body.c_str());

    Json::Value val;
    json_util::unserialize(body, &val);
    std::cout << "姓名:" << val["姓名"].asString() << std::endl;
    std::cout << "年龄:" << val["年龄"].asInt() << std::endl;
    int sz = val["成绩"].size();
    for (int i = 0; i < sz; i++)
    {
        std::cout << "成绩: " << val["成绩"][i].asFloat() << std::endl;
    }
}

void str_test()
{
    std::string str = ",...,,123,234,,,,,345,,,,";
    std::vector<std::string> arry;
    string_util::split(str, ",", arry);
    for (auto s : arry)
    {
        DLOG("%s", s.c_str());
    }
}

void file_test()
{
    std::string filename = "./makefile";
    std::string body;
    file_util::read(filename, body);

    std::cout << body << std::endl;
}

void db_test()
{
    user_table ut(HOST, USER, PASS, DBNAME, PORT);
    Json::Value user;
    user["username"] = "xiaoming";
    user["password"] = "123456";
    std::cout << ut.login(user) << std::endl;
    // bool ret = ut.insert(user);
    // ut.select_by_id(1, user);
    // ut.select_by_name("小明", user);
    // ut.win(3);
    // ut.lose(1);
    // ut.lose(2);
    // std::string body;
    // json_util::serialize(user, &body);
    // std::cout << body << std::endl;
}

void online_test()
{
    online_manager om;
    wsserver_t::connection_ptr conn;
    uint64_t uid = 2;
    om.enter_game_room(uid, conn);
    if (om.is_in_game_room(uid))
        DLOG("IN GAME HALL");
    else
        DLOG("NOT IN GAME HALL");
    om.exit_game_room(uid);
    if (om.is_in_game_room(uid))
        DLOG("IN GAME HALL");
    else
        DLOG("NOT IN GAME HALL");
}

int main()
{
    // online_test();
    // user_table ut(HOST, USER, PASS, DBNAME, PORT);
    // online_manager om;
    // room_manager rm(&ut, &om);
    // room_ptr rp = rm.create_room(10, 20);
    // matcher mc(&rm, &ut, &om);
    return 0;
}
