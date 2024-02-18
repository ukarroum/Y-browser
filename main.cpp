#include <iostream>

#include "URL.hpp"

void show(const std::string& body)
{
    bool in_tag = false;
    for(auto c: body)
    {
        if(c == '<')
            in_tag = true;
        else if(c == '>')
            in_tag = false;
        else if(!in_tag)
            std::cout << c;
    }
}

void load(const URL& url)
{
    show(url.request());
}

int main(int argc, char**argv)
{
    if(argc != 2)
    {
        std::cout << "Usage: " << std::endl;
        std::cout << "ybrowser <url>" << std::endl;
        return 64;
    }

    URL url{argv[1]};

    load(url);

    return 0;
}