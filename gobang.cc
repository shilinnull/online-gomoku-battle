#include "server.hpp"
#include "Test.hpp"

#define HOST "127.0.0.1"
#define PORT 3306
#define USER "root"
#define PASS "123456"
#define DBNAME "gobang"



int main()
{
    gobang_server server(HOST, USER, PASS, DBNAME, PORT);
    server.run(8085);

    return 0;
}
