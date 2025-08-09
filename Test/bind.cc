#include <iostream>
#include <string>
#include <functional>

void Print(const std::string& s, int num)
{
    printf("%s: %d\n", s.c_str(), num);
}

int main()
{
    auto f = std::bind(Print, "hello", std::placeholders::_1);

    f(100);
    return 0;
}
