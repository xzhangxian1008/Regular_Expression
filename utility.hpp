#ifndef UTIL
#define UTIL

#include <iostream>
#include <string>

inline void error(std::string str)
{
    std::cout << "Lexical analyzer error:" << str << std::endl;
    exit(-1);
}

inline void warn(std::string str)
{
    std::cout << "Lexical analyzer warning:" << str << endl;
}

inline void print(string str, bool flag = true)
{
    bool off_on = flag;
    if (off_on)
        std::cout << str << std::endl;
}

#endif