#include <stdarg.h>

#include <iostream>

using namespace std;

void TestPrint(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    while(fmt != "e")
    {
        cout<<fmt<<endl;
        fmt=va_arg(args,char*);
    }
    //注意，这里va_list并不以NULL为结束，所以不要进行循环的找值
    //使用时应以找到某个值为目标进行操作
    va_end(args);
}

int main(int argc, char **argv)
{
    TestPrint("a","b","c","d", "e");

    return 0;
}